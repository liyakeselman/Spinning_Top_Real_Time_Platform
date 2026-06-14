#ifndef MOTORS_H
#define MOTORS_H

// ================= GLOBAL CONSTANTS ==============================================

// define control pins for motor driver 1
constexpr int M1_A_IA = 2;
constexpr int M1_A_IB = 1;
constexpr int M1_B_IA = 19;
constexpr int M1_B_IB = 3;

// define control pins for motor driver 2
constexpr int M2_A_IA = 0;
constexpr int M2_A_IB = 4;
constexpr int M2_B_IA = 5;
constexpr int M2_B_IB = 18;

// ================= FUNCTIONS =====================================================

void backward();
void forward();
void setupMotorDrivers();
void motor_u2_left();
void motor_u2_right();
void motor_u2_forward();
void motor_u2_backward();
void motor_u2_stop();


#endif
