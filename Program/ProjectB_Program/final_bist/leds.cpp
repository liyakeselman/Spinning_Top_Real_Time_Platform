#pragma once

// ================= INCLUDES ======================================================

#include <Arduino.h>
#include "structureManager.h" 
#include "leds.h"

// ================= FUNCTIONS =====================================================

void setupLeds() {
  // set pinmodes for leds
  Serial.println(F(""));
  Serial.println(F(" setting pin modes to leds ..."));
  for (int i = 0; i < LEDS_NUM; ++i) {
    pinMode(gpio_led[i], OUTPUT);
    delay(10);
  }
  delay(1000);
  Serial.println(F(" setting pin modes to leds - done"));
  delay(1000);
  is_done_setup_leds = true;
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F(" DONE : setup leds"));
}

void turnOnLeds() {
  for (int i = 0; i < LEDS_NUM; ++i) {
    digitalWrite(gpio_led[i], LOW);
  }
}

void turnOffLeds() {
  for (int i = 0; i < LEDS_NUM; ++i) {
    digitalWrite(gpio_led[i], HIGH);
  }
}

