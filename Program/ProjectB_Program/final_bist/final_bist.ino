// ================= INCLUDES ======================================================

#include <Arduino.h>
#include <math.h>
#include "communicationManagement.h"
#include "setupManager.h"
#include "rotation_and_display.h"
#include "angle_tracking.h"  

// ================= GLOBAL CONSTANTS ==============================================

#define STOP_CONFIRM_MS  800 // 300 for debugging with rotation with hand, 800 for real (fast) simulation 

// Frequency Constrains
const uint32_t FAST_MODE_INT_MS =  20;  // [ms]
const uint32_t SAMP_INT_MS      =  20;  // [ms]
const uint32_t MPU_INT_MS       =  5;   // [ms]
const uint32_t HAMMER_INT_MS    =  75;  // [ms]

// ================= GLOBAL OBJECTS INIT ===========================================

// buffers init (for write)
masterPacket dataBuffer[BUFFER_SIZE]; // buffer to store chunk of CSV lines before writing to file
masterPacket dataBufferWrite[BUFFER_SIZE];

// shared structure init
Data dataAll = {
  .currentTime = 0,
  .previousTime = 0,

  // luna
  .luna_dist = 0,
  .luna_strength = 0,
  .luna_temp = 0,

  // mpu
  .mpu_gyroX = 0,.mpu_gyroY = 0,.mpu_gyroZ = 0,.mpu_tot_gyro = 0,
  .mpu_accX = 0,.mpu_accY = 0,.mpu_accZ = 0, .mpu_tot_acc = 0,
  .mpu_temp = 0,
  .mpuAngle = 0, 
  
  // vl
  .vldistance = {0, 0, 0},
  .VLsAngle = 0,
  
  // rotation
  .spinCount = 0,
  .fusedAngle = 0, 

  // status
  .currentTopDir = 0,
  .currentHammerDir = 0,
  .currentSystemMode = 0,
  .currentTopMode = 0,
  .currentHammerMode = 0
};

// semaphore for writer tasks (mpuTask(5ms), rangeTask(20[ms] (3Vls+Luna), onWebSocketEvent(20[ms]))
SemaphoreHandle_t dataWriteMutex = nullptr; // mutex for writers-tasks only (to shared dataAll structure) 

// ================= dataAll SEQLOCK ================================================

volatile uint32_t dataSeq = 0;
void dataWriteBegin() {
  if (dataWriteMutex) xSemaphoreTake(dataWriteMutex, portMAX_DELAY); // waits untill the writer ends
  __atomic_add_fetch(&dataSeq, 1u, __ATOMIC_SEQ_CST);
} // aquires dataWriteMutex

void dataWriteEnd() {
  __atomic_add_fetch(&dataSeq, 1u, __ATOMIC_SEQ_CST);
  if (dataWriteMutex) xSemaphoreGive(dataWriteMutex); // release the queue to the next writer
} // releases dataWriteMutex

static inline void readDataSnapshot(Data &out) {
  uint32_t s1, s2;
  int retry = 0;
  do {
    s1 = __atomic_load_n(&dataSeq, __ATOMIC_SEQ_CST);
    if (s1 & 1u) { vTaskDelay(1); continue; } // if someone writes now - wait a moment
    out = dataAll;
    s2 = __atomic_load_n(&dataSeq, __ATOMIC_SEQ_CST);
    if (++retry > 10) break; // protection: if after 10 tries we didnt succeed - use what you have and continue
  } while (s1 != s2 || (s2 & 1u)); 
} // the seqlock (do "optimistic read()")

// ================= GLOBAL VARIABLES INIT =========================================

// TEST state
static float     test_angle      = 0.0f;
static float     test_omega      = 12.0f; // (fake angle) [rad/s]
static uint32_t  test_prev_us    = 0;
static float     test_prev_angle = 0.0f;

// for Android App
ACTIVITY current_activity = CONNECT;

// global state variables
String Mode = "Off";
bool is_left = false; 
bool is_right = false; 
bool is_forward = false; 
bool is_backward = false; 

// global samples count helper variables
int bufferIndex = 0;
int bufferCount = 0;

// tasks variables
static TaskHandle_t loggerTaskHandle = nullptr;
static SemaphoreHandle_t fileMutex = nullptr;
static volatile bool writePending = false;
static volatile int writeCount = 0;

TickType_t loop_lastWakeTime;
const TickType_t loop_period = pdMS_TO_TICKS(1); // 1[ms] per loop

static volatile uint32_t lastSensorUpdate = 0;
volatile bool isNewVLDataAvailable = false;

// ================= DEBUG SETTINGS ================================================

static constexpr bool TEST_MODE = false;      // IMPORTANT: TEST_MODE=false for real use!! TEST_MODE=true for debugging

static constexpr bool DEBUG_PRINT = false;    // FOR DEBUG (used in printDebugInfo func)
static constexpr bool ENABLE_HAMMER = false;  // FOR DEBUG (testing without hammerTask) 
static constexpr bool ENABLE_POV = true;      // FOR DEBUG (for canceling POV)

// Debug print timing
static uint32_t lastDebugPrint = 0;           // FOR DEBUG (used in printDebugInfo func)
static constexpr uint32_t DEBUG_INT_MS = 500; // FOR DEBUG (used in printDebugInfo func)

// ================= OTHER SETTINGS ================================================

// Timing
int iter = 0;
unsigned long previousSensorsSampTime = 0;
unsigned long currentSensorsSampTime = 0; 
unsigned long previousIterTime[BUFFER_SIZE] = { 0 };
unsigned long currentIterTime[BUFFER_SIZE] = { 0 }; 
unsigned long iterTime[BUFFER_SIZE] = { 0 }; 
unsigned long timestamps[BUFFER_SIZE] = { 0 };

unsigned long timestamp_global = 0;

// Setup flags
bool is_done_setup_leds = false; 
bool is_done_setup_tfLuna = false; 
bool is_done_setup_VL = false; 
bool is_done_setup_mpu = false; 
bool is_done_setup_motors = false; 
bool is_done_setup_all = false;

// Connected components
bool is_tfLuna_connected = false; 
bool is_mpu_connected = true;
bool is_all_VL_connected = true; 
bool is_md1_chA_connected = false; 
bool is_md1_chB_connected = false; 
bool is_md2_chA_connected = false; 
bool is_md2_chB_connected = true; 
bool is_md1_connected = is_md1_chA_connected || is_md1_chB_connected;
bool is_md2_connected = is_md2_chA_connected || is_md2_chB_connected;


// Debug exports for computing digit and see from ChoiceActivity (for debug)
volatile int g_bucket10 = 0;
volatile uint8_t g_digit = 0;

// State machine
State currentState = STOPPED;

// ================= FreeRTOS ======================================================

/* ==============================================================================
 *  MPU TASK (core 1 , Prio 2) - runs every 5[ms] 
 * ============================================================================== */
void mpuTask(void *pvParameters) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t taskPeriod = pdMS_TO_TICKS(MPU_INT_MS);

  uint32_t prev_us = micros();

  while (true) {
    uint32_t now_us = micros();
    float dt = (now_us - prev_us) * 1e-6f;
    prev_us = now_us;

    dataWriteBegin();

    if (is_mpu_connected) readMPUValue();
    dataAll.previousTime = dataAll.currentTime;
    dataAll.currentTime = now_us;
    float dummyVL[3] = {
      dataAll.vldistance[0],
      dataAll.vldistance[1],
      dataAll.vldistance[2]
    };
    // update the fused angle
    updateAngle(dataAll.mpu_gyroZ, dt, isNewVLDataAvailable, dummyVL, -1.0f); // -1.0f = Luna disabled
    // update fields of mpu_tot_acc and mpu_tot_gyro in the shared data sturct
    dataAll.mpu_tot_acc =  sqrt(dataAll.mpu_accX*dataAll.mpu_accX+dataAll.mpu_accY*dataAll.mpu_accY+dataAll.mpu_accZ*dataAll.mpu_accZ);
    dataAll.mpu_tot_gyro = sqrt(dataAll.mpu_gyroX*dataAll.mpu_gyroX+dataAll.mpu_gyroY*dataAll.mpu_gyroY+dataAll.mpu_gyroZ*dataAll.mpu_gyroZ);
    if (isNewVLDataAvailable) isNewVLDataAvailable = false; // we turn off the flag after using
    // update the field spinCount in the shared data sturct
    dataAll.spinCount = getspinCount();
    // update the field mpuAngle  in the shared data sturct
    dataAll.mpuAngle  = getMpuAngleRad(); // [rad] pure gyro angle for app and .bin/csv and mpuActivity
    // update the field fusedAngle in the shared data sturct
    dataAll.fusedAngle = getFusedCurrAngleRad(); // [rad] fused angle for led display
    
    dataWriteEnd();

    lastSensorUpdate = millis();
    vTaskDelayUntil(&lastWakeTime, taskPeriod);
  }
}

/* ==============================================================================
 * RANGE TASK (core 0 , Prio 1) - runs every 20[ms] 
 * ============================================================================== */
void rangeTask(void *pvParameters) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t taskPeriod = pdMS_TO_TICKS(FAST_MODE_INT_MS);

  while (true) {
    dataWriteBegin();

    if (is_all_VL_connected) {
      read3VL();
      isNewVLDataAvailable = true; // this flag signs to MPUTask that there is new detail
    }

    if (is_tfLuna_connected) {
      readLunaValue();
    }

    dataAll.VLsAngle =
      estimateAngleFromVL(dataAll.vldistance[0], dataAll.vldistance[1], dataAll.vldistance[2]); // [rad] ([-π, π])
    // maybe need to convert to [0, 2π) with getVLsAngleRad()
    dataWriteEnd();

    vTaskDelayUntil(&lastWakeTime, taskPeriod);
  }
}

/* ==============================================================================
 * SAMPLING TASK (core 1 , Prio 2) - runs every 20[ms] 
 * sends to application every 50ms (for telemetry and spibcount indication)
 * sends to application every 600ms (for moving top/hammer indication)
 * ============================================================================== */
void samplingTask(void *pvParameters) {
  while(!is_done_setup_all) {vTaskDelay(FAST_MODE_INT_MS / portTICK_PERIOD_MS);}
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t taskPeriod = pdMS_TO_TICKS(SAMP_INT_MS);

  // Serial.println("Starting Task ..."); // debug (delete)
  while (true) {
    Data snap;
    readDataSnapshot(snap); //snapshot of the last updated data in structure from buffer (dataAll structure)

    uint32_t globalSamples = (uint32_t)bufferIndex * BUFFER_SIZE + iter;

    if (globalSamples < DESIRED_NUM_SAMPLES) {
      uint32_t now_ms = millis();
      // buffer and timestamp calculations for csv
      int bucket10 = 0;
      uint8_t digit = 0;
      computeBucketAndDigit(snap.spinCount, bucket10, digit);

      currentSensorsSampTime = now_ms; // (now_ms is millis() return value) 

      timestamp_global += currentSensorsSampTime - previousSensorsSampTime;
      previousSensorsSampTime = currentSensorsSampTime;
      timestamps[iter] = timestamp_global;

      currentIterTime[iter] = currentSensorsSampTime;
      iterTime[iter] = currentIterTime[iter] - previousIterTime[iter];
      if (iter + 1 < BUFFER_SIZE) {
        previousIterTime[iter + 1] = currentIterTime[iter];
      }

      // (1) filling masterPacket
      // writing the line to csv format (to SD)
      int lineIdx = iter;
      masterPacket& cp = dataBuffer[lineIdx]; // reference to socket in buffer (cp = copy packet)
      memset(&cp, 0, sizeof(masterPacket));

      uint32_t iter_global = (uint32_t)bufferIndex * BUFFER_SIZE + lineIdx;

      // masterPacket filling (data for app)
      cp.iter_global = globalSamples;
      cp.curr_activity_type = (uint8_t)current_activity;
      cp.raw_data_samp_time = snap.currentTime / 1000.0f; // time of raw data sampling (for the mpu it is the time where the angle was calculated) (micros to millis)
      
      cp.binLineWr_time = timestamps[lineIdx]/1.0f;       // time of samplingTask occurance (millis, float)
      cp.iterTime_binlineIdx = iterTime[lineIdx]/1.0f;    // millis, float
    
      // copy luna data
      cp.luna_dist = snap.luna_dist;
      cp.luna_strength = snap.luna_strength;
      cp.luna_temp = snap.luna_temp;
      // copy mpu data 
      cp.mpuAngle = snap.mpuAngle * (180.0f / PI);   // [rad ]to [deg]
      cp.mpu_tot_gyro = snap.mpu_tot_gyro;           // [rad/sec]
      cp.mpu_tot_acc = snap.mpu_tot_acc;             // [m/sec^2]
      cp.mpu_gyroX = snap.mpu_gyroX;                 // [rad/sec]
      cp.mpu_gyroY = snap.mpu_gyroY;                 // [rad/sec]
      cp.mpu_gyroZ = snap.mpu_gyroZ;                 // [rad/sec]]
      cp.mpu_accX = snap.mpu_accX;                   // [m/sec^2]
      cp.mpu_accY = snap.mpu_accY;                   // [m/sec^2]
      cp.mpu_accZ = snap.mpu_accZ;                   // [m/sec^2]
      // copy vl data
      cp.vl1_dist = snap.vldistance[0];              // [cm]
      cp.vl2_dist = snap.vldistance[1];              // [cm]
      cp.vl3_dist = snap.vldistance[2];              // [cm]
      cp.VLsAngle = snap.VLsAngle * (180.0f / PI);   // [rad ]to [deg]
      // copy rotation data
      cp.fusedAngle = snap.fusedAngle * (180.0f/PI); //[rad] to [deg]
      cp.spinCount = snap.spinCount;
      cp.bucket10 = bucket10;
      cp.digit = (int)digit;
      
      /*
      cp.currentTopDir     = currentTopDir;
      cp.currentHammerDir  = currentHammerDir;
      cp.currentSystemMode = currentSystemMode;
      cp.currentTopMode    = currentTopMode;
      cp.currentHammerMode = currentHammerMode;
      */

      // synced version is better than accessing globals
      cp.currentTopDir     = snap.currentTopDir;
      cp.currentHammerDir  = snap.currentHammerDir;
      cp.currentSystemMode = snap.currentSystemMode;
      cp.currentTopMode    = snap.currentTopMode;
      cp.currentHammerMode = snap.currentHammerMode;


      // (2) sending to app in binary format
      static uint32_t lastWsSendMs = 0;
      uint32_t nowMs = millis();
      /* NOTE: 
         samplingTask still occurs every 20[ms] (for csv)
         but
         sends to application every 50[ms] (for telemetry and spibcount indication)
         sends to application every 600[ms] (for moving top/hammer indication)
       */
      uint32_t wsIntervalMs = 50;
      if (current_activity == OPERATE || current_activity == MOVE_TOP || current_activity == CONNECT) {
        wsIntervalMs = 600;
        lastWsSendMs = nowMs;
      }
      if (nowMs - lastWsSendMs >= wsIntervalMs) {
        BinUpdateApp(cp);
        lastWsSendMs = nowMs; 
      }
    

      // (3) buffer management and moving to LoggerTask 
      if (bufferCount < (lineIdx + 1)) bufferCount = (lineIdx + 1);

      iter++;
      if (iter == BUFFER_SIZE) {
        const int n = bufferCount;
        while (writePending) vTaskDelay(1); // wait untill the logger finishes previous cycle
        memcpy(dataBufferWrite, dataBuffer, n * sizeof(dataBuffer[0]));
        writeCount = n;
        writePending = true;
        if (loggerTaskHandle) xTaskNotifyGive(loggerTaskHandle); // here LoggerTask gets tick 
        bufferCount = 0;
        bufferIndex++;
        iter = 0;
        previousIterTime[0] = millis();
      }
    }
    // accurately waiting to next cycle
    vTaskDelayUntil(&lastWakeTime, taskPeriod);
  }
}

/* ==============================================================================
 * HAMMER TASK (core 1 , Prio 1) - 
 * runs in current implementation every 75[ms].
 * 50[ms] might be risky for batteries 
 * ============================================================================== */
void hammerTask(void* pvParameters) {
  const TickType_t hammerDelay = pdMS_TO_TICKS(HAMMER_INT_MS);
  while (true) {
    backward();
    vTaskDelay(hammerDelay);
    forward();
    vTaskDelay(hammerDelay);
  }
  // for future operation if currentHammerDir variable is updated here so you must use sync mechanism 
}

/* ==============================================================================
 * LOGGER TASK (core 1 , Prio 1) - wakes up when sampling task finishes 256 samples.
 * runs only after setup (to not log to csv when setup has not finished)
 * ============================================================================== */
void loggerTask(void* pvParameters) {
  while(!is_done_setup_all) { vTaskDelay(pdMS_TO_TICKS(SAMP_INT_MS)); } 
  while (true) {
    // wait to notification from SamplingTask that there are ready buffer (chunk of 256)
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
    const int n = writeCount;
    if (n > 0) {
      if (fileMutex) xSemaphoreTake(fileMutex, portMAX_DELAY);
      
      // the fastest way is to write the whole block at once
      // (const uint8_t*)dataBufferWrite is a pointer to beginning of the array
      // n*sizeof(masterPacket) is the total size in Bytes
      file.write((const uint8_t*)dataBufferWrite, n * sizeof(masterPacket)); // write to file system
      // no file.flush() here ! it will be executed only when we finish the measure, and inside the loop

      if (fileMutex) xSemaphoreGive(fileMutex);
    }
    
    // releasing the flag - lets samplingTask continue to run
    writePending = false;
  }
}

// ================= DEBUG PRINT FUNCTION ==========================================

void printDebugInfo(const Data &snap) {
  if (!DEBUG_PRINT) return;
  
  uint32_t now = millis();
  if (now - lastDebugPrint < DEBUG_INT_MS) return;
  lastDebugPrint = now;
  
  Serial.println(); // empty line before the debug prints
  
  // Line 1: Gyroscope + Accelerometer
  Serial.printf("[Sensors] Gyro: X=%6.2f Y=%6.2f Z=%6.2f | Acc: X=%5.2f Y=%5.2f Z=%5.2f\n",
    snap.mpu_gyroX, snap.mpu_gyroY, snap.mpu_gyroZ,
    snap.mpu_accX, snap.mpu_accY, snap.mpu_accZ
  );
  
  // Line 2: VL distances
  Serial.printf("[Sensors] VL: 0=%3.0fcm 1=%3.0fcm 2=%3.0fcm",
    snap.vldistance[0], snap.vldistance[1], snap.vldistance[2]
  );
  
  // Add Luna if connected
  if (is_tfLuna_connected && snap.luna_dist > 0) {
    Serial.printf(" | Luna: %3dcm (str:%3d)", snap.luna_dist, snap.luna_strength);
  }
  Serial.println();
  
  // Line 3: Angle tracking // FIX?
  float fusedAngleDeg = snap.fusedAngle * 180.0f / PI; 
  int currentDigit = (snap.spinCount / 10) % 10;
  
  Serial.printf("[Angle]   %.3f rad (%6.1f°) | Spin: %4d | Digit: %d",
    snap.fusedAngle, fusedAngleDeg, snap.spinCount, currentDigit
  );
  
  // Show state
  Serial.printf(" | State: %s\n", 
    currentState == ROTATING ? "ROTATING" : "STOPPED"
  );
  
  // Line 4: Temperatures
  Serial.printf("[Temp]    MPU: %5.1f°C", snap.mpu_temp);
  if (is_tfLuna_connected) {
    Serial.printf(" | Luna: %5.1f°C", snap.luna_temp);
  }
  Serial.println();
  
  // Line 5: Angle comparison (debug)
  if (snap.VLsAngle != -1.0f) {  // valid VL angle
    float VLsAngleDeg = snap.VLsAngle * 180.0f / PI; // [deg]
    float mpuAngleDeg = snap.mpuAngle * 180.0f / PI; // [deg]
    float diff = wrapToPi((snap.mpuAngle - snap.VLsAngle)) * 180.0f / PI; // [deg]
    Serial.printf("[Debug]   Gyro angle: %6.1f° | VL angle: %6.1f° | Diff: %+6.1f°\n",
      mpuAngleDeg, VLsAngleDeg, diff
    );
  }
  
  Serial.println("---------------------------------------------------");
}

// ================= SETUP =========================================================

void setup() {
  bufferIndex = 0;
  callInitTest();
  wellcomeMessage();

  callSetupSensorsAndLeds(); 
  
  // Initialize angle tracking
  initAngleTracking();
  Serial.println(F("[SETUP] Angle tracking initialized"));
  // Mark that motors need Serial port to be shut (so there won't be monitoring)
  WarningsToUser();
  // Setup motor drivers
  setupMotorDrivers();
  // Create file (high-resolution, for offline analysis)
  prepareBinaryFile();

  // Create mutex (writers lock)
  fileMutex = xSemaphoreCreateMutex();

  // Create tasks
  xTaskCreatePinnedToCore(mpuTask, "MPUTask", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(rangeTask, "RangeTask", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(samplingTask, "SamplingTask", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(loggerTask, "LoggerTask", 8192, NULL, 1, &loggerTaskHandle, 1);
  
  // Hammer task 
  if (ENABLE_HAMMER) { 
    xTaskCreatePinnedToCore(hammerTask, "HammerTask", 2048, NULL, 1, NULL, 1);
    Serial.println(F("[SETUP] Hammer enabled"));
  } else {
    Serial.println(F("[SETUP] Hammer DISABLED for testing")); 
  }
  
  dataWriteMutex = xSemaphoreCreateMutex(); // create the lock for data
  is_done_setup_all = true;
  loop_lastWakeTime = xTaskGetTickCount();

  Serial.println(F("[SETUP] Complete with Mutex Protection!"));
  Serial.println("[NOTE] Ending Serial to free UART0 pins !"); 
  delay(200);
  Serial.end(); // critical for hammers !!!
}

// ================= LOOP ==========================================================

void loop() {
  Data snap;
  readDataSnapshot(snap);

  printDebugInfo(snap);

  const uint32_t now_ms = millis();

  /* ============================================================================
   * Testing/ Debug Mode (TEST_MODE=true) debug mode (Fake spin count)
   * ============================================================================ */
  if (TEST_MODE) {
    if (test_prev_us == 0) test_prev_us = micros();
    uint32_t now_us = micros();
    float dt_sim = (now_us - test_prev_us) * 1e-6f;
    test_prev_us = now_us;

    test_prev_angle = test_angle;
    test_angle = wrapTo2Pi(test_angle + test_omega * dt_sim);

    snap.fusedAngle = test_angle;
    snap.mpu_gyroZ = test_omega;

    // Fake spin count
    if (test_prev_angle > 5.5f && test_angle < 0.8f) {
      snap.spinCount++;
    }
    
    currentState = ROTATING;
  }

  /* ============================================================================
   *  Real Performance Mode (not testing/debug/sim) (TEST_MODE=false)
   *  The State Machine (Real spin count)
   * ============================================================================ */
  if (!TEST_MODE) { 
    static uint32_t lowGyroSinceMs = 0;
    if (fabs(snap.mpu_gyroZ) >= GYROZ_THRESHOLD_FOR_ROTATION_START) {
      lowGyroSinceMs = 0;
      if (currentState == STOPPED) {
        sendStartSpinningMassege();
        currentState = ROTATING;
      }
    } else if (fabs(snap.mpu_gyroZ) <= GYROZ_THRESHOLD_FOR_ROTATION_STOP) {
        if (currentState == ROTATING) {
          if (lowGyroSinceMs == 0) lowGyroSinceMs = millis();

          // Stop only if gyro stayed low for 300ms (when debug) and comment resetAngleTracking();
          // Stop only if gyro stayed low for 800ms (real simulatiton) and uncomment resetAngleTracking();
          if (millis() - lowGyroSinceMs >= STOP_CONFIRM_MS) {
            sendStopSpinningMassege();
            currentState = STOPPED;
            lowGyroSinceMs = 0;

            // flush happens only when stopped
            if (fileMutex) xSemaphoreTake(fileMutex, portMAX_DELAY);
            file.flush(); // flush ensures that all what is remained in memory is physically written to Flash in the end of the spin
            if (fileMutex) xSemaphoreGive(fileMutex);

            // Reset angle tracking when stopped
            //resetAngleTracking();// if you want to debug comment it and change STOP_CONFIRM_MS from 800 to 300
        }
     }
    } else  lowGyroSinceMs = 0;
  }

  /* ============================================================================
   * Compute Display Digit
   * ============================================================================ */
    int bucket10 = 0;
    uint8_t digit = 0; 
    computeBucketAndDigit(snap.spinCount, bucket10, digit); 
    //g_bucket10 = bucket10;    // debug
    //g_digit = (uint8_t)digit; // debug

  /*
    * // when debug POV and you want to force display of constant digit (e.g., '1')
    * cancel "uint8_t digit = 0; computeBucketAndDigit(snap.spinCount, bucket10, digit);" lines
    * and add "uint8_t digit = 1;" line
  */
  
  /* ============================================================================
   * Testing/ Debug Mode (TEST_MODE=true) print (test computed digit to display)
   * ============================================================================ */
  if (TEST_MODE) {
    static uint32_t tPrint = 0;
    if (millis() - tPrint > 200) {
      tPrint = millis();
      Serial.printf("theta=%.2f omega=%.2f spin=%d bucket=%d digit=%d\n",
        snap.fusedAngle, snap.mpu_gyroZ, snap.spinCount, bucket10, (int)digit);
    }
  }

  /* ============================================================================
   *  LED display
   * Since fused is not reliable because of VLs fallbacks 
   * we define theta_disp=snap.mpuAngle and displayTimedSpinPhase(...,snap.mpuAngle).
   * In case it is reliable we define:
   * theta_disp=snap.fusedAngle and displayTimedSpinPhase(...,snap.fusedAngle)
   * ============================================================================ */
if (ENABLE_POV) {
  if (currentState == ROTATING) {
    uint32_t now_us = micros();
    float theta_disp = snap.mpuAngle;   //[rad] 
    //float theta_disp = snap.mpuAngle; //[rad]

    if (snap.currentTime != 0) {
      float age_s = (now_us - snap.currentTime) * 1e-6f;
      theta_disp += snap.mpu_gyroZ * age_s;
    }

    while (theta_disp < 0.0f) theta_disp += 2.0f * M_PI;
    while (theta_disp >= 2.0f * M_PI) theta_disp -= 2.0f * M_PI;

    displayTimedSpinPhase((uint8_t)digit, theta_disp, snap.mpu_gyroZ); // wrapper to draw with theta_disp and convert snap.mpu_gyroZ to void
    //displayTimedSpinPhase((uint8_t)digit, theta_disp, snap.fusedAngle); 
  } else {
    turnOffLeds();
  }
}
  vTaskDelayUntil(&loop_lastWakeTime, loop_period);
}

