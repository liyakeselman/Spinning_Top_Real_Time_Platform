#pragma once

// ================= INCLUDES ======================================================

#include <VL53L0X.h>
#include <Arduino.h>
#include "vl.h"

// ================= GLOBAL CONSTANTS ==============================================

const uint8_t vl_addrs[3] = {LOX1_ADDRESS, LOX2_ADDRESS, LOX3_ADDRESS};
const uint8_t xshut_pins[3] = {SHT_LOX1, SHT_LOX2, SHT_LOX3};
const uint32_t VL_TIMING_BUDGET = 20000;  // set high-speed mode: 20[ms] timing budget

const uint32_t VL_INTERVAL_MS   = 20;  // 20 [ms] interval (high-speed mode)

// ================= GLOBAL OBJECTS ================================================

VL53L0X vl[3] = {VL53L0X(), VL53L0X(), VL53L0X()};

// ================= GLOBAL DATA ARRAYS ============================================

uint16_t currentReadings[NUM_VL] = {0, 0, 0};
bool VLInitialized[NUM_VL] = {false, false, false};
uint16_t measure_reading;

// ================= FUNCTIONS =====================================================

void boot_unboot_3VL(bool to_boot) {
  for (int i = 0; i < 3; ++i) {
    VLInitialized[i] = to_boot;
  }
}

void setID() {
  // reset all sensors
  for (int i = 0; i < 3; ++i) {
    digitalWrite(xshut_pins[i], LOW);
  }
  delay(10);
  
  // bring up sensors one by one
  digitalWrite(xshut_pins[0], HIGH); 
  delay(10);
  vl[0].init(); 
  if (!vl[0].init()) {
    Serial.println("ERROR : Failed to initialize LOX1");
    while (1);
  }
  vl[0].setAddress(vl_addrs[0]); 
  delay(10);
  
  digitalWrite(xshut_pins[1], HIGH); 
  delay(10);
  vl[1].init(); 
  if (!vl[1].init()) {
    Serial.println("ERROR : Failed to initialize LOX2");
    while (1);
  }
  vl[1].setAddress(vl_addrs[1]); 
  delay(10);
  
  digitalWrite(xshut_pins[2], HIGH); 
  delay(10);
  vl[2].init(); 
  if (!vl[2].init()) {
    Serial.println("ERROR : Failed to initialize LOX3");
    while (1);
  }
  vl[2].setAddress(vl_addrs[2]); 
  delay(10);
  
  // set measurement timing budget and start continuous
  for (int i = 0 ; i < 3; ++i) {
    vl[i].setTimeout(10); // configure max waiting time to not slow the for-loop of read3VL
    vl[i].setMeasurementTimingBudget(VL_TIMING_BUDGET); // configure sampling time
    vl[i].startContinuous(VL_INTERVAL_MS); // Start continuous back-to-back mode (here it starts to measure in background)
  }
  
  boot_unboot_3VL(true);
}

void setup3VL() {
  Serial.println(F(""));
  Serial.println(F(" setting pin modes to 3 VL53L0XV2 sensors ..."));
  delay(1000);
  
  for (int i = 0; i < 3; i++) {
    pinMode(xshut_pins[i], OUTPUT);
    digitalWrite(xshut_pins[i], LOW);
  }
  delay(10);
  Serial.println(F(" done"));
  delay(1000);

  Serial.println(F(" initializing VL53L0XV2 ..."));
  setID();
  
  bool failed = !VLInitialized[0] || !VLInitialized[1] || !VLInitialized[2];
  if (!failed) {
    Serial.printf("VL1 addres: ");Serial.print(vl_addrs[0],HEX);
    Serial.print("\nVL2 addres: ");Serial.print(vl_addrs[1],HEX);
    Serial.print("\nVL3 addres: ");Serial.print(vl_addrs[2],HEX);
    Serial.print("\n");
  }
  else {
    Serial.println(F(" ERROR : failed to initialize one or more VL53L0X sensors"));
    while (1);
  }

  Serial.println(F(" FINISHED : setup VL53L0XV2"));
  delay(1000);
  is_done_setup_VL = true;
}

void read3VL() {
  for (int i = 0; i < NUM_VL; i++) {
    if (!VLInitialized[i]) {
      //dataAll.vldistance[i] = 120; // fallback
      continue; // we use readRangeContinuous func that simply takes the current value from the register without waiting to new measurement to finish (to avoid blocking of readRangeContinuousMillimeters() )
    }
    // Read result as fast as possible
    measure_reading = vl[i].readRangeContinuousMillimeters();

    // Clamp to valid range (120[cm] max)
    // Performance is best within 1.2 meters, with long-range mode extending up to 2 meters
    if (vl[i].timeoutOccurred() || measure_reading > 1200) { // if the measurement wasn't ready or timeout was occured
      dataAll.vldistance[i] = 120; 
    } else {
      dataAll.vldistance[i] = measure_reading / 10;  // mm to cm
    }

    // Store in buffer
    currentReadings[i] = dataAll.vldistance[i];
  }
}
