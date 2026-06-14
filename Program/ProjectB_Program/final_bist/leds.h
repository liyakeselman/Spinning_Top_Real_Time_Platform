#ifndef LEDS_H
#define LEDS_H

#pragma once

// ================= GLOBAL CONSTANTS ==============================================

// define control pins leds
constexpr int LEDS_NUM = 8;
enum LEDS { LED1 = 32,
            LED2 = 33,
            LED3 = 25,
            LED4 = 26,
            LED5 = 27,
            LED6 = 14,
            LED7 = 12,
            LED8 = 17 };
constexpr int gpio_led[LEDS_NUM] = { LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8 };  // GPIO pins

// ================= FUNCTIONS =====================================================

void setupLeds();
void turnOnLeds();
void turnOffLeds();

#endif
