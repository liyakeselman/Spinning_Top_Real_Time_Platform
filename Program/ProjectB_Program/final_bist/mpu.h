#ifndef MPU_H
#define MPU_H

#pragma once

// ================= INCLUDES ======================================================

#include <MPU6050_tockn.h>
#include <Wire.h>
#include "structureManager.h" 
#define DIM 4 // cartesic + magnitude

// ================= FUNCTIONS =====================================================

float normalize_angle(float raw_angle);
void setupMPU();
void readMPUValue();


#endif
