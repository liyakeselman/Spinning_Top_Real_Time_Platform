#pragma once

// ================= INCLUDES ======================================================

#include <Arduino.h>
#include <cmath>
#include "mpu.h"

// ================= GLOBAL CONSTANTS ==============================================

#define MPU6050_ADDRESS 0x68 // for MPU6050 (I2C)

#define MPU6050_INT_ENABLE_REG 0x38
#define DIM 4 // 3 dimentions + linear magnitude
#define GRAVITY_ACC 9.80665 // [m/sec^2]

// ================= GLOBAL OBJECTS ================================================

MPU6050 mpu6050(Wire);  // I2C object

// ================= FUNCTIONS =====================================================

float normalize_angle(float raw_angle) {
  float normalizedAngle = fmod(raw_angle, 360.0f);
  // handle case of negative values (if rotation to the opposite side)
  if (normalizedAngle < 0) {
      normalizedAngle += 360.0f;
  }
  return normalizedAngle;
}

void setupMPU() {
  Serial.println(F(" Initializing MPU with tockn library ..."));
  mpu6050.begin(); 
  delay(200);
  Serial.println(F(" Calibrating MPU ... Please do not move the top"));
  mpu6050.calcGyroOffsets(true);
  delay(200);

  Serial.println(F(" FINISHED : MPU Setup Complete!"));
  delay(1000);
  is_done_setup_mpu = true;
}

void readMPUValue() {
 // take all data from the mpu6050 sensor
  mpu6050.update();

  dataAll.mpu_accX = mpu6050.getAccX()* 9.80665f; // converted to [m/sec^2] from G units
  dataAll.mpu_accY = mpu6050.getAccY()* 9.80665f; // converted to [m/sec^2] from G units
  dataAll.mpu_accZ = mpu6050.getAccZ()* 9.80665f; // converted to [m/sec^2] from G units
  dataAll.mpu_tot_acc = sqrtf(dataAll.mpu_accX*dataAll.mpu_accX+dataAll.mpu_accY*dataAll.mpu_accY+dataAll.mpu_accZ*dataAll.mpu_accZ);

  // Store in dataAll (with bias correction)
  dataAll.mpu_gyroX = mpu6050.getGyroX() * (PI / 180.0f); // converted from [∘/sec] to[rad/sec] 
  dataAll.mpu_gyroY = mpu6050.getGyroY() * (PI / 180.0f); // converted from [∘/sec] to[rad/sec] 
  dataAll.mpu_gyroZ = mpu6050.getGyroZ() * (PI / 180.0f); // converted from [∘/sec] to[rad/sec] 
  dataAll.mpu_tot_gyro = sqrtf(
  dataAll.mpu_gyroX*dataAll.mpu_gyroX+dataAll.mpu_gyroY*dataAll.mpu_gyroY+dataAll.mpu_gyroZ*dataAll.mpu_gyroZ);
  dataAll.mpu_temp = mpu6050.getTemp(); // [degC]

  dataAll.mpuAngle= mpu6050.getAngleZ() * (PI / 180.0f); // converted from [deg] to [rad]
  
}






