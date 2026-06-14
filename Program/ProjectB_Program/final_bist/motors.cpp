#pragma once

// ================= INCLUDES ======================================================

#include <Arduino.h>
#include "structureManager.h" 
#include "motors.h"

// ================= FUNCTIONS =====================================================

// handle motor-driver movement helper functions 

// move backward 
void backward() {
  // NOTE: this implementation is not full. we need to do it for both channels and for both motors !
  if (is_md2_connected) {
    if (is_md2_chB_connected) {
      digitalWrite(M2_B_IA, HIGH);
      digitalWrite(M2_B_IB, LOW);
    }
  }
}

// move forward
void forward() {
  // NOTE: this implementation is not full. we need to do it for both channels and for both motors !
  if (is_md2_connected) {
    if (is_md2_chB_connected) {
      digitalWrite(M2_B_IA, LOW);
      digitalWrite(M2_B_IB, HIGH);
    }
  }
}


// setup helper function : setup motor-drivers 
void setupMotorDrivers() {
  // set pin modes for motor-drivers
  if (is_md1_connected) {
    // set pinmodes for motor driver 1
    //pinMode(M1_A_IA, OUTPUT);
    //pinMode(M1_A_IB, OUTPUT);
    //pinMode(M1_B_IA, OUTPUT);
    //pinMode(M1_B_IB, OUTPUT);
    // setup motor 1 : reset control pins of motor-drivers to LOW
    //digitalWrite(M1_A_IA, LOW);
    //digitalWrite(M1_A_IB, LOW);
    //digitalWrite(M1_B_IA, LOW);
    //digitalWrite(M1_B_IB, LOW);
  }
  if (is_md2_connected) {
    // set pinmodes for motor driver 2
    pinMode(M2_A_IA, OUTPUT);
    pinMode(M2_A_IB, OUTPUT);
    pinMode(M2_B_IA, OUTPUT);
    pinMode(M2_B_IB, OUTPUT);
    // setup motor 2 : reset control pins of motor-drivers to LOW
    digitalWrite(M2_A_IA, LOW);
    digitalWrite(M2_A_IB, LOW);
    digitalWrite(M2_B_IA, LOW);
    digitalWrite(M2_B_IB, LOW);
  }
  delay(1000);
  is_done_setup_motors = true;
}


// operate hammer helper functions : (used when received websocket command from app)

void motor_u2_left() {
  // Rotate Motor Driver 2 - Motor A clockwise
  /*digitalWrite(M2_A_IA, HIGH);
  digitalWrite(M2_A_IB, LOW);*/

  // Rotate Motor Driver 2 - Motor B clockwise
  /*digitalWrite(M2_B_IA, HIGH);
  digitalWrite(M2_B_IB, LOW);*/

  //delay(500);
  //motor_u2_stop();
}

void motor_u2_right() {
  // Rotate Motor Driver 2 - Motor A counter-clockwise
  /*digitalWrite(M2_A_IA, LOW);
  digitalWrite(M2_A_IB, HIGH);*/

  // Rotate Motor Driver 2 - Motor B counter-clockwise
  /*digitalWrite(M2_B_IA, LOW);
  digitalWrite(M2_B_IB, HIGH);*/

  //delay(500);
  //motor_u2_stop();
}

void motor_u2_forward() {
  // Rotate Motor Driver 2 - Motor A clockwise
  /*digitalWrite(M2_A_IA, HIGH);
  digitalWrite(M2_A_IB, LOW);*/

  // Rotate Motor Driver 2 - Motor B clockwise
  digitalWrite(M2_B_IA, LOW);
  digitalWrite(M2_B_IB, HIGH);

  //delay(500);
  //motor_u2_stop();
}

void motor_u2_backward() {
  // Rotate Motor Driver 2 - Motor A clockwise
  /*digitalWrite(M2_A_IA, LOW);
  digitalWrite(M2_A_IB, HIGH);*/

  // Rotate Motor Driver 2 - Motor B clockwise
  digitalWrite(M2_B_IA, HIGH);
  digitalWrite(M2_B_IB, LOW);

  //delay(500);
  //motor_u2_stop();
}

void motor_u2_stop() {
  // Rotate Motor Driver 2 - Motor A clockwise
  digitalWrite(M2_A_IA, LOW);
  digitalWrite(M2_A_IB, LOW);

  // Rotate Motor Driver 2 - Motor B clockwise
  digitalWrite(M2_B_IA, LOW);
  digitalWrite(M2_B_IB, LOW);
}

