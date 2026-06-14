#ifndef VL_H
#define VL_H

#pragma once

// ================= INCLUDES ======================================================

#include <Adafruit_Sensor.h>
#include "Adafruit_VL53L0X.h"
#include <VL53L0X.h>
#include "structureManager.h" 

// ================= GLOBAL CONSTANTS ==============================================

constexpr int NUM_VL = 3;

// control pins for 3 VL53L0XV2 sensors to communication
constexpr int LOX1_ADDRESS = 0x21;
constexpr int LOX2_ADDRESS = 0x22;
constexpr int LOX3_ADDRESS = 0x23;

// control pins for 3 VL53L0XV2 sensors to shutdown
constexpr int SHT_LOX1 = 13;
constexpr int SHT_LOX2 = 23;
constexpr int SHT_LOX3 = 15;


// ================= GLOBAL OBJECTS ================================================

extern VL53L0X vl[NUM_VL]; // I2C object

// ================= GLOBAL DATA ARRAYS ============================================

extern uint16_t currentReadings[NUM_VL]; // hold a measurement for each vl sensor
extern bool VLInitialized[NUM_VL];       // hold boot state for each vl sensor

// ================= FUNCTIONS =====================================================

void boot_unboot_3VL(bool to_boot);
void setID();
void setup3VL();
void read3VL();
#endif
