#ifndef SETUPMANAGER_H
#define SETUPMANAGER_H

#pragma once

// ================= INCLUDES ======================================================

#include "mpu.h"
#include "luna.h"
#include "vl.h"
#include "leds.h"
#include "motors.h"

// ================= FUNCTIONS =====================================================

void serialInit();
void wellcomeMessage();
void WarningsToUser();
void setupData();
void callInitTest();
void callSetupSensorsAndLeds();

#endif
