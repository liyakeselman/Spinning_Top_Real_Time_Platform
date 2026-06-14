#ifndef LUNA_H
#define LUNA_H

#pragma once

// ================= INCLUDES ======================================================

#include "structureManager.h" 

// ================= GLOBAL CONSTANTS ==============================================

// define control pins for tfLuna sensor communication
constexpr int GPIO_17_TX2 = 17;  // ESP_LUNA_LED8_TXD
constexpr int GPIO_16_RX2 = 16;  // LUNA_ESP_RXD
constexpr int HEADER = 0x59;     // constant for tf-luna operation

// ================= GLOBAL OBJECTS ================================================

extern HardwareSerial tfLuna;  // using serial port 2 (UART2)

// ================= GLOBAL VARIABLES ==============================================

extern int dist, strength, check, uart[9];
extern float temprature; 
extern bool canReadFromTFLuna; // a flag to see if data is available for reading from tfLuna sensor

// ================= FUNCTIONS =====================================================

void setupTFluna();
void readLunaValue();

#endif
