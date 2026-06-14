#pragma once

// ================= INCLUDES ======================================================

#include <Arduino.h>
#include "luna.h"

// ================= GLOBAL OBJECTS ================================================

HardwareSerial tfLuna(2);  // using serial port 2 (UART2)

// ================= GLOBAL VARIABLES ===============================================

// global variables : tf-luna operation 
int dist, strength, check, uart[9];
float temprature; 
bool canReadFromTFLuna = false; // a flag indicate if data is available for reading from tfLuna sensor


// ================= FUNCTIONS =======================================================

void setupTFluna() {
  // set pinmodes for tfLuna
  Serial.println(F(""));
  Serial.println(F(" setting pin modes to tfLuna sensor ..."));
  delay(1000);
  pinMode(GPIO_16_RX2, INPUT);
  delay(10);
  pinMode(GPIO_17_TX2, OUTPUT);
  delay(10);
  Serial.println(F(" setting pin modes to tfLuna sensor - done"));
  delay(1000);
  // setup tf-luna (initialize the tfLuna communication)
  Serial.println(F(""));
  Serial.println(F(" setup tfLuna begin :  Initializing Serial2 port (UART2) ..."));
  tfLuna.begin(BAUD_RATE, SERIAL_8N1, GPIO_16_RX2, GPIO_17_TX2);
  delay(2000);  // wait for Serial to initialize
  while (!tfLuna) {
    Serial.println(F(""));
    Serial.println(F(" ERROR : Initializing Serial2 port (UART2) FAILED, trying to connect again ..."));
    delay(2000);  // wait for Serial to initialize
    return;
  }
  Serial.println(F(" SUCCESS for initializing Serial2 port (UART2)"));
  delay(1000);

  // notes
  Serial.println(F(""));
  Serial.println(F(""));
  // NOTE: distance units for tf-luna sensor are [cm]
  Serial.println(F(" distance from tf-luna sensor is unreliable when strength < 100 or strength = 65535 (overexposure)"));
  delay(1000);
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F(" DONE : setup tfLuna"));
  delay(1000);
  is_done_setup_tfLuna = true;
}


void readLunaValue() {
  // Check if data is available from the TF-Luna sensor
  if (tfLuna.available()) {
    canReadFromTFLuna = true;
    if (tfLuna.read() == HEADER) {
      uart[0] = HEADER;
      if (tfLuna.read() == HEADER) {
        uart[1] = HEADER;
        for (int i = 2; i < 9; i++) {
          uart[i] = tfLuna.read();
        }
        check = uart[0] + uart[1] + uart[2] + uart[3] + uart[4] + uart[5] + uart[6] + uart[7];
        if (uart[8] == (check & 0xff)) {
          dist = uart[2] + uart[3] * 256;        //calculate distance value
          strength = uart[4] + uart[5] * 256;    //calculate signal strength value
          temprature = uart[6] + uart[7] * 256;  //calculate chip temprature
          temprature = temprature / 8 - 256;
          
          // store samples in structure object
          dataAll.luna_dist = dist;
          dataAll.luna_strength= strength;
          dataAll.luna_temp = temprature;
        }
      }
    }
  } else {
    canReadFromTFLuna = false;
  }
}

