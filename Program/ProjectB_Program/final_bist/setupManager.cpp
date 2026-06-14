#pragma once
// ================= INCLUDES ======================================================
#include <Arduino.h>
#include "communicationManagement.h"
#include "setupManager.h"

#define I2C_FREQ 400000 // 400[kHz]

// ================= FUNCTIONS =====================================================

// initiate Serial port
void serialInit() {
  Serial.begin(BAUD_RATE);
  Serial.println(F(""));
  Serial.println(F(" Initializing serial port ..."));
  delay(2000);  // Wait for Serial to initialize
  while (!Serial) {
    Serial.println(F(" ERROR : Initializing serial port FAILED, Retrying ..."));
    delay(2000);  // Wait for Serial to initialize
    return;
  }
  Serial.println(F(" Serial port initialized successfully"));
  delay(1000);
}

// introduction message
void wellcomeMessage() {
  Serial.println(F(""));
  Serial.println(F(" This is a test to check the real-time telemetry live on Android application and offline on CSV logging ..."));
  delay(1000);
  Serial.println(F(""));
  Serial.println(F(" Preparing to setup units ..."));
  delay(1000);
}

// Serial port does not work with some GPIOs
void WarningsToUser() {
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F(" ###### WARNINGS ###### "));
  Serial.println(F(" (1) we will shut down the Serial port"));
  delay(1000);
  Serial.println(F(" NOTE !!!"));
  Serial.println(F(" Serial port will be off, so printings to Serial Monitor will not be available !"));
  delay(1000);
  Serial.println(F(" next operations: "));
  Serial.println(F(" (2) we will set pin modes to motor-drivers"));
  Serial.println(F(" (3) we will reset control pins of motor-drivers to LOW"));
  Serial.println(F(" (4) we will start loop iterations"));
  delay(1000);
}

// setup dataAll structure
void setupData() {
    dataAll.currentTime = micros(); // micros
    dataAll.previousTime = 0; // micros
    
    // mpu
    dataAll.mpuAngle = 0;
    dataAll.mpu_gyroZ = 0;
    dataAll.mpu_temp= 0;
    
    dataAll.mpu_accX = 0;
    dataAll.mpu_accY = 0;
    dataAll.mpu_accZ = 0;

    // luna
    dataAll.luna_dist = -1;
    dataAll.luna_strength = 0;
    dataAll.luna_temp = 0;

    // vl
    dataAll.vldistance[0] = -1;
    dataAll.vldistance[1] = -1;
    dataAll.vldistance[2] = -1;
    dataAll.VLsAngle = 0; 

    // resultant
    dataAll.fusedAngle = 0;
    dataAll.mpu_tot_acc = 0;
    dataAll.mpu_tot_gyro = 0;


    dataAll.spinCount = 0;
}


// calls initialization functions 
void callInitTest() {
  serialInit();
  LittleFSinit(); 
  wifiDualModeInit();
  initializeWebServer();
}

// calls setup function to each sensor 
void callSetupSensorsAndLeds() {
  setupLeds();
  setupTFluna();
  // I2C
  Wire.begin();Wire.setClock(I2C_FREQ);delay(200); // for mpu and vl
  setupMPU();
  // scanning I2C since maybe the MPU6050 is fake and not 0x68
  Serial.println("Scanning I2C bus...");
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at address: 0x");
      Serial.println(address, HEX);
    }
  }
  setup3VL();
  
}



