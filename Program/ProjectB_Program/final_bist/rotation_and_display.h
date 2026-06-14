#ifndef ROTATION_AND_DISPLAY_H
#define ROTATION_AND_DISPLAY_H

#pragma once
#include <Arduino.h>

// ================= GLOBAL CONSTANTS ==============================================

static constexpr int SPINS_PER_BUCKET = 10;  // 10 spins per digit

// ================= HELPER FUNCTIONS ==============================================

// Compute bucket and digit from spin count
// LOGIC: Always count up 0→1→2→3... regardless of direction
static inline void computeBucketAndDigit(int spinCount, int &bucket10, uint8_t &digit) {
  // Take absolute value and apply same logic for both directions
  int absCount = abs(spinCount);
  if (absCount == 0) {
    bucket10 = 0;
    digit = 0;
  } else {
    // 1-10 → bucket 0, 11-20 → bucket 1, etc.
    bucket10 = (absCount - 1) / SPINS_PER_BUCKET;
    digit = (uint8_t)(bucket10 % 10);
  }
}

// ================= DISPLAY FUNCTIONS =============================================

void displayspinCountOnLeds(int number);

// Display functions for each digit (0-9) - these DRAW the digits!
void display_0(float angle);
void display_1(float angle);
void display_2(float angle);
void display_3(float angle);
void display_4(float angle);
void display_5(float angle);
void display_6(float angle);
void display_7(float angle);
void display_8(float angle);
void display_9(float angle);

// main display function - calls the appropriate display_X function
void displayNumber(int number, float angle);

// Wrapper for compatibility with existing code
void displayTimedSpinPhase(uint8_t digit, float theta_now, float omega_now);

extern uint8_t rotatedPattern;

#endif