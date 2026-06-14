// ================= INCLUDES ======================================================

#include "angle_tracking.h"
#include <math.h>

// ================= INTERNAL STATE ================================================

static float currentAngle = 0.0f;       // [rad] fused output angle in [0, 2π) the final angle to display/app wrapped to  [0, 2π)
static int spinCount = 0;               // number of full rotations, counted from pure gyro only (without VL)
static float internalFusedAngle = 0.0f; // [rad] internal fused angle: gyro integration + accumulated VL correction
static float mpuAngle_pure = 0.0f;      // [rad] pure gyro angle for spin counting only

static float VLsAngle = 0.0f;           // [rad] VL-based estimate

// Luna pulse detection state
static float lastLunaDistance = 100.0f;
static uint32_t lastLunaPulseMs = 0;
static bool lunaInPulseZone = false;

// ================= INTERNAL FUNCTIONS ============================================

/*
 * Detect Luna pulse (marker seen once per rotation)
 * Applies strong phase correction when pulse detected
 */
static void detectLunaPulse(float lunaDistance) {
  const uint32_t now = millis();
  
  // Hysteresis state machine
  if (!lunaInPulseZone) {
    // Entering pulse zone?
    if (lunaDistance < LUNA_PULSE_THRESHOLD && 
        lastLunaDistance >= LUNA_PULSE_THRESHOLD) {
      
      // Debounce (minimum 100ms between pulses)
      if (now - lastLunaPulseMs >= 100) {
        lunaInPulseZone = true;
        lastLunaPulseMs = now;
        
        // PULSE DETECTED! Apply phase correction
        // Assumption: marker is at angle = LUNA_ANGLE_ON_TOP
        float targetAngle = LUNA_ANGLE_ON_TOP;
        float error = wrapToPi(internalFusedAngle - targetAngle);
        
        mpuAngle_pure = internalFusedAngle; // save to local
        
        // Strong correction (30% of error)
        internalFusedAngle -= LUNA_CORRECTION_STRENGTH * error;
        internalFusedAngle = wrapTo2Pi(internalFusedAngle);
        
        #ifdef DEBUG_ANGLE_TRACKING
        Serial.printf("[Luna] PULSE! angle=%.1f° error=%.1f° corrected=%.1f°\n",
          (internalFusedAngle + error) * 180.0f / PI,
          error * 180.0f / PI,
          mpuAngle_pure * 180.0f / PI
        );
        #endif
      }
    }
  } else {
    // Exiting pulse zone?
    if (lunaDistance >= LUNA_PULSE_HYSTERESIS) {
      lunaInPulseZone = false;
    }
  }
  
  lastLunaDistance = lunaDistance;
} // not used in this implementation, no magnet marker (additional hardware required)

// ================= FUNCTIONS =====================================================

void initAngleTracking() { 
  currentAngle = 0.0f; // fused
  spinCount = 0;
  mpuAngle_pure = 0.0f;
  internalFusedAngle = 0.0f; 
  VLsAngle = 0.0f;
  Serial.println(F("[AngleTracking] Initialized"));
} // called once in setup()


// generate fused angle (mpu angle with correction from other sensors (in this case only VLs))
void updateAngle(float gyroZ, float dt, bool useVLCorrection, const float vlDistances[3], float lunaDistance) {
  // Protect against timing glitches that can create a fake large angle jump.
  // The MPU task is expected to run every 5[ms] in mpuTask
  if (dt <= 0.0f || dt > 0.05f)  dt = 0.005f;
  
  // Dead zone - ignore small gyro values (noise/bias) 
  const float GYRO_DEAD_ZONE = 0.15f;  // ~8.6 [deg/s]
  if (fabs(gyroZ) < GYRO_DEAD_ZONE) gyroZ = 0.0f;  // ~8.6 [deg/s]
  
  
  /* ======================================================================
   * Step 1: Pure gyro angle for spin counting only
   * ====================================================================== */
  // mpuAngle_pure is not corrected by VL.
  // Therefore spinCount cannot be affected by VL noise or VL jumps.
  mpuAngle_pure += gyroZ * dt;  // [rad/sec] * [sec] = [rad]
  while (mpuAngle_pure >= 2.0f * PI) {
    mpuAngle_pure -= 2.0f * PI;
    spinCount++;
    #ifdef DEBUG_ANGLE_TRACKING
    Serial.printf("[AngleTracking] Spin count: %d\n", spinCount);
    #endif
  }
  while (mpuAngle_pure < 0.0f) {
    mpuAngle_pure += 2.0f * PI;
    spinCount--;
  }
  /* ======================================================================
   * Step 2: Fused angle integration for display/LED
   * ====================================================================== */
  // internalFusedAngle is the fused internal angle.
  // It accumulates gyro integration and also keeps VL corrections over time.
  internalFusedAngle += gyroZ * dt;
  internalFusedAngle = wrapTo2Pi(internalFusedAngle); // keep fused internal angle bounded before correction.

  /* ======================================================================
   * Step 3: VL-based drift correction for fused angle only
   * ====================================================================== */
  if (useVLCorrection) {
    VLsAngle = estimateAngleFromVL(vlDistances[0], vlDistances[1], vlDistances[2]); // [rad], [-π, π]
    if (VLsAngle != -1.0f) {
      // convert VL angle from [-π, π] to [0, 2π).
      float angleVL_positive = (VLsAngle < 0.0f) ? (VLsAngle + 2.0f * PI) : VLsAngle;
      // shortest angular error on a circle.
      float err = wrapToPi(internalFusedAngle - angleVL_positive);
      // apply gentle accumulated correction to the fused angle.
      // this corrects drift over time, but does not touch spinCount.
      internalFusedAngle -= VL_CORRECTION_WEIGHT * err;
      // keep fused angle bounded after correction.
      internalFusedAngle = wrapTo2Pi(internalFusedAngle);
    }
  }
  /* ======================================================================
   * Step 4: Output
   * ====================================================================== */
  currentAngle = internalFusedAngle;
}

float getMpuAngleRad() { 
   return wrapTo2Pi(mpuAngle_pure); 
}

void resetAngleTracking() {  
  internalFusedAngle = 0.0f;
  spinCount = 0;
  currentAngle = 0.0f;
  mpuAngle_pure = 0.0f;
  Serial.println(F("[AngleTracking] Reset! spinCount = 0"));
} // not used in implementation without magnets

float getFusedCurrAngleRad() { 
  return currentAngle;
}

int getspinCount() {
  return spinCount;
}

void resetspinCount() {
  spinCount = 0;
  Serial.println(F("[AngleTracking] Spin count reset"));
} // func is not used

float estimateAngleFromVL(float d0, float d1, float d2) { // input in [cm]
  // validate inputs
  if (d0 <= 0.0f || d1 <= 0.0f || d2 <= 0.0f) {
    return -1.0f;  // invalid
  }
  
  // additional validity check (reasonable range) 
  if (d0 >= 120.0f || d1 >= 120.0f || d2 >= 120.0f) {
    return -1.0f;  // probably measurement error
  }
  
  // VL sensors positioned at 0°, 120°, 240° on the spinning top
  const float a0 = 0.0f;
  const float a1 = 2.0f * PI / 3.0f;   // 120° = 2π/3
  const float a2 = 4.0f * PI / 3.0f;   // 240° = 4π/3
  
  // vector sum method
  // each sensor contributes a vector pointing in its direction
  // with magnitude proportional to measured distance
  float x = d0 * cosf(a0) + d1 * cosf(a1) + d2 * cosf(a2);
  float y = d0 * sinf(a0) + d1 * sinf(a1) + d2 * sinf(a2);

  // resultant angle
  return atan2f(y, x);  // returns angle in range [-π, π] 
}

float getVLsAngleRad() {  
   return wrapTo2Pi(VLsAngle); 
}

// ================= DEBUG FUNCTIONS ===============================================

#ifdef DEBUG_ANGLE_TRACKING
void printAngleState() {
  Serial.printf("[AngleTracking] angle=%.2f° gyro=%.2f° VL=%.2f° spin=%d\n",
    currentAngle * 180.0f / PI,
    internalFusedAngle * 180.0f / PI,
    VLsAngle * 180.0f / PI,
    spinCount
  );
}
#endif