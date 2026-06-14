// ================= INCLUDES ======================================================

#include <cmath>
#include <Arduino.h>
#include "leds.h" 
#include "rotation_and_display.h"

// ================= HELPER FUNCTIONS ==============================================

static inline float wrap2pi(float a) {
  while (a < 0) a += 2.0f * M_PI;
  while (a >= 2.0f * M_PI) a -= 2.0f * M_PI;
  return a;
}

static inline bool inWindow(float th, float a, float b) {
  // th,a,b in [0,2pi)
  if (a <= b) return (th >= a && th <= b);
  return (th >= a || th <= b); // crosses 0
}

// ================= DIGIT PATTERN (7-segment style) ===============================

uint8_t rotatedPattern = 0;

void displayspinCountOnLeds(int number) {
  if (number < 0 || number > 9) return;
  
  for (int i = 0; i < LEDS_NUM-1; i++) {
    if (rotatedPattern & (1 << i)) {
      digitalWrite(gpio_led[i], LOW);  // ON
    } else {
      digitalWrite(gpio_led[i], HIGH); // OFF
    }
  }
}

/* ================= DIGIT DRAWING FUNCTIONS =======================================
 * Each function draws a 7-segment style digit by changing the LED pattern
 * at different angles during rotation
 * WINDOW (section) VERSION: 0°-15° (narrower window for sharper visibility)
 * NOTE: "float angle" parameter should be in radians
 * ================================================================================= */

/* digit = '0'
    1111111
    1000001
    1111111
*/
void display_0(float angle) { 
  if (angle >= 0*(M_PI/180) && angle < 5*(M_PI/180)) {
    rotatedPattern = 0b1111111;
  }
  else if (angle >= 5*(M_PI/180) && angle < 10*(M_PI/180)) {
    rotatedPattern = 0b1000001;
  }
  else if (angle >= 10*(M_PI/180) && angle < 15*(M_PI/180)) {
    rotatedPattern = 0b1111111;
  }
  else {
    rotatedPattern = 0b0000000;
  }
  displayspinCountOnLeds(0);
}

/* digit = '1'
    0000000
    0000001
    1111111
*/
void display_1(float angle) {
  if (angle >= 0*(M_PI/180) && angle < 10*(M_PI/180)) {
    rotatedPattern = 0b1111111; // vertical stroke
  }
  else if (angle >= 10*(M_PI/180) && angle < 15*(M_PI/180)) {
    rotatedPattern = 0b0000001; // short horizontal stroke
  }
  else {
    rotatedPattern = 0b0000000; // off
  }
  displayspinCountOnLeds(1);
}

/* digit = '2'
    1111001
    1001001
    1001111
*/
void display_2(float angle) {
  if (angle >= 0*(M_PI/180) && angle < 5*(M_PI/180)) {
      rotatedPattern = 0b1001111;
    }
    else if (angle >= 5*(M_PI/180) && angle < 10*(M_PI/180)) {
      rotatedPattern = 0b1001001;
    }
    else if (angle >= 10*(M_PI/180) && angle < 15*(M_PI/180)) {
      rotatedPattern = 0b1111001;
    }
    else {
      rotatedPattern = 0b0000000;
    }
  displayspinCountOnLeds(2);
}

/* digit = '3'
    1001001
    1001001
    1111111
*/
void display_3(float angle) {
  if (angle >= 0*(M_PI/180) && angle < 5*(M_PI/180)) {
      rotatedPattern = 0b1111111;
    }
    else if (angle >= 5*(M_PI/180) && angle < 10*(M_PI/180)) {
      rotatedPattern = 0b1001001;
    }
    else if (angle >= 10*(M_PI/180) && angle < 15*(M_PI/180)) {
      rotatedPattern = 0b1001001;
    }
    else {
      rotatedPattern = 0b0000000;
    }
  displayspinCountOnLeds(3);
}

/* digit = '4'
    0001111
    0001000
    1111111
*/
void display_4(float angle) {
  if (angle >= 0*(M_PI/180) && angle < 5*(M_PI/180)) {
    rotatedPattern = 0b1111111;
  }
  else if (angle >= 5*(M_PI/180) && angle < 10*(M_PI/180)) {
    rotatedPattern = 0b0001000;
  }
  else if (angle >= 10*(M_PI/180) && angle < 15*(M_PI/180)) {
    rotatedPattern = 0b0001111;
  }
  else {
    rotatedPattern = 0b0000000;
  }
  displayspinCountOnLeds(4);
}

/* digit = '5'
    1001111
    1001001
    1111001
*/
void display_5(float angle) {
  if (angle >= 0*(M_PI/180) && angle < 5*(M_PI/180)) {
    rotatedPattern = 0b1111001;
  }
  else if (angle >= 5*(M_PI/180) && angle < 10*(M_PI/180)) {
    rotatedPattern = 0b1001001;
  }
  else if (angle >= 10*(M_PI/180) && angle < 15*(M_PI/180)) {
    rotatedPattern = 0b1001111;
  }
  else {
    rotatedPattern = 0b0000000;
  }
  displayspinCountOnLeds(5);
}

/* digit = '6'
    1111111
    1001001
    1111001
*/
void display_6(float angle) {
  if (angle >= 0*(M_PI/180) && angle < 5*(M_PI/180)) {
    rotatedPattern = 0b1111001;
  }
  else if (angle >= 5*(M_PI/180) && angle < 10*(M_PI/180)) {
    rotatedPattern = 0b1001001;
  }
  else if (angle >= 10*(M_PI/180) && angle < 15*(M_PI/180)) {
    rotatedPattern = 0b1111111;
  }
  else {
    rotatedPattern = 0b0000000;
  }
  displayspinCountOnLeds(6);
}

/* digit = '7'
    0001001
    0001001
    1111111
*/
void display_7(float angle) {
  if (angle >= 0*(M_PI/180) && angle < 5*(M_PI/180)) {
    rotatedPattern = 0b1111111;
  }
  else if (angle >= 5*(M_PI/180) && angle < 10*(M_PI/180)) {
    rotatedPattern = 0b0001001;
  }
  else if (angle >= 10*(M_PI/180) && angle < 15*(M_PI/180)) {
    rotatedPattern = 0b0001001;
  }
  else {
    rotatedPattern = 0b0000000;
  }
  displayspinCountOnLeds(7);
}

/* digit = '8'
    1111111
    1001001
    1111111
*/
void display_8(float angle) {
  if (angle >= 0*(M_PI/180) && angle < 5*(M_PI/180)) {
    rotatedPattern = 0b1111111;
  }
  else if (angle >= 5*(M_PI/180) && angle < 10*(M_PI/180)) {
    rotatedPattern = 0b1001001;
  }
  else if (angle >= 10*(M_PI/180) && angle < 15*(M_PI/180)) {
    rotatedPattern = 0b1111111;
  }
  else {
    rotatedPattern = 0b0000000;
  }
  displayspinCountOnLeds(8);
}

/* digit = '9'
    1001111
    1001001
    1111111
*/
void display_9(float angle) {
  if (angle >= 0*(M_PI/180) && angle < 5*(M_PI/180)) {
    rotatedPattern = 0b1111111;
  }
  else if (angle >= 5*(M_PI/180) && angle < 10*(M_PI/180)) {
    rotatedPattern = 0b1001001;
  }
  else if (angle >= 10*(M_PI/180) && angle < 15*(M_PI/180)) {
    rotatedPattern = 0b1001111;
  }
  else {
    rotatedPattern = 0b0000000;
  }
  displayspinCountOnLeds(9);
}

// ================= MAIN DISPLAY FUNCTION ===============================

void displayNumber(int number, float angle) {
  // Normalize angle to [0, 2π)
  angle = wrap2pi(angle);
  
  // Get the tens digit
  int tens = (number / 10) % 10;
  
  switch (tens) {
    case 0: display_0(angle); break;
    case 1: display_1(angle); break;
    case 2: display_2(angle); break;
    case 3: display_3(angle); break;
    case 4: display_4(angle); break;
    case 5: display_5(angle); break;
    case 6: display_6(angle); break;
    case 7: display_7(angle); break;
    case 8: display_8(angle); break;
    case 9: display_9(angle); break;
    default: display_0(angle); break;
  }
}

// ================= WRAPPER FOR COMPATIBILITY ===========================

// This wrapper allows existing code to work without changes
// It's called from final_bist.ino with the digit already computed
void displayTimedSpinPhase(uint8_t digit, float theta_now, float omega_now) {
  (void)omega_now;  // Not needed for the drawing functions
  
  // Just call displayNumber with a fake spinCount that produces this digit
  // Since we only care about the tens digit, we use digit * 10
  displayNumber(digit * 10, theta_now);
}