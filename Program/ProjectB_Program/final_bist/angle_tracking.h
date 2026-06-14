#ifndef ANGLE_TRACKING_H
#define ANGLE_TRACKING_H

#pragma once

// ================= INCLUDES ======================================================

#include <Arduino.h>
#include "structureManager.h"

// ================= GLOBAL CONSTANTS ==============================================

// VL correction weight (how much to trust VL vs Gyro)
// 0.0 = pure gyro, 0.1 = aggressive VL correction
constexpr float VL_CORRECTION_WEIGHT = 0.02f;  // 2% correction per cycle

// Luna settings
constexpr bool USE_LUNA_AS_PULSE = true;         // use Luna for marker detection
constexpr bool USE_LUNA_IN_FUSION = false;       // include Luna in angle fusion
constexpr float LUNA_ANGLE_ON_TOP = 0.0f;        // [rad] luna mounting angle (adjust!)
constexpr float LUNA_PULSE_THRESHOLD = 40.0f;    // [cm] distance to detect marker
constexpr float LUNA_PULSE_HYSTERESIS = 60.0f;   // [cm] exit threshold
constexpr float LUNA_CORRECTION_STRENGTH = 0.3f; // phase lock gain

// ================= FUNCTIONS =====================================================

/**
 * Initialize angle tracking
 * Call once in setup()
 */
 // initialize angle tracking.
void initAngleTracking(); // call once in setup

/**
 * Update angle estimation from gyro + VL + Luna
 * Call every sensor cycle (rangeTask interval is 20[ms])
 * 
 * @param gyroZ Angular velocity [rad/s]
 * @param dt Time step [seconds]
 * @param vlDistances Array of 3 VL distances [cm]
 * @param lunaDistance Luna distance [cm] (optional, use -1 to disable)
 */
 // generate fused angle (mpu angle with correction from other sensors (here only fused with VLs))
void updateAngle(float gyroZ, float dt, bool useVLCorrection, const float vlDistances[3], float lunaDistance = -1.0f);

// get current angle estimate [rad] Range: [0, 2π)
float getFusedCurrAngleRad();

// get current spin count (number of full rotations)
int getspinCount();

// estimate angle from 3 VL sensors (geometric method)
// returns angle in range [-π, π] or -1.0 if invalid
// d0, d1, d2 are 3 vl readings in [cm]
float estimateAngleFromVL(float d0, float d1, float d2);

// get current VLsAngle (Range: [0, 2π))
float getVLsAngleRad();

// reset angle tracking manualy.
// not used in this implementation 
// should be used in when magnets are reseting (additional hardware for marker detection)
void resetAngleTracking();

// reset spin count (optional - for manual reset)
//not used in this implementation
void resetspinCount();

// ================= HELPER FUNCTIONS ==============================================

// wrap angle to  [0, 2π)
static inline float wrapTo2Pi(float angle) {
  angle = fmodf(angle, 2.0f * PI);
  if (angle < 0.0f) angle += 2.0f * PI;
  return angle;
}

// wrap angle to  [-π, π]
static inline float wrapToPi(float angle) {
  
  angle = fmodf(angle + PI, 2.0f * PI);
  if (angle < 0.0f) angle += (2.0f * PI);
  return angle - PI;
}

// get mpu reading for angle
float getMpuAngleRad();

#endif // ANGLE_TRACKING_H