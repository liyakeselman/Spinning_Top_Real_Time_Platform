/* --------------------------------------------------- include files ---------------------------------------------------------*/
#include <Arduino.h>
/* --------------------------------------------------- global variables ---------------------------------------------------------*/
// baud rate for simulation
const int BAUD_RATE = 115200;

// motor full speed for pwm
const int MOTOR_FULL_SPEED = 255;
// motor stop speed for pwm
const int MOTOR_STOP = 0;

// control pins for motor driver 1 - channel A
const int M1_A_IA = 2;
const int M1_A_IB = 1;
// control pins for motor driver 1 - channel B
const int M1_B_IA = 19;
const int M1_B_IB = 3;
// control pins for motor driver 2 - channel A
const int M2_A_IA = 0;
const int M2_A_IB = 4;
// control pins for motor driver 2 - channel B
const int M2_B_IA = 5;
const int M2_B_IB = 18; 

// counter for the loop
int loop_iter = 0;

/* --------------------------------------------------- reset motor states----------------------------------------------------------*/

void reset_md_states() {
  // Initialize motor driver 1 control pins to LOW
  digitalWrite(M1_A_IA, LOW);
  digitalWrite(M1_A_IB, LOW);
  digitalWrite(M1_B_IA, LOW);
  digitalWrite(M1_B_IB, LOW);
  delay(1000);
  // Initialize motor driver 2 control pins to LOW
  digitalWrite(M2_A_IA, LOW);
  digitalWrite(M2_A_IB, LOW);
  digitalWrite(M2_B_IA, LOW);
  digitalWrite(M2_B_IB, LOW);
  delay(1000);
}

/* --------------------------------------------------- move motors functions ----------------------------------------------------------*/
/*
 * NOTE: you can change MOTOR_FULL_SPEED to HIGH and MOTOR_STOP to LOW ! 
 * NOTE: you can change digitalWrite to analogWrite !
 */
// motor 1 channel a 
void backward_MD1_CH_A() {
  digitalWrite(2, MOTOR_FULL_SPEED);
  digitalWrite(1, MOTOR_STOP);
}
void forward_MD1_CH_A() {
  digitalWrite(1, MOTOR_FULL_SPEED);
  digitalWrite(2, MOTOR_STOP);
}

// motor 1 channel b
void backward_MD1_CH_B() {
  digitalWrite(19, MOTOR_FULL_SPEED);
  digitalWrite(3, MOTOR_STOP);
}

void forward_MD1_CH_B() {
  digitalWrite(3, MOTOR_FULL_SPEED);
  digitalWrite(19, MOTOR_STOP);
}

// motor 2 channel a 
void backward_MD2_CH_A() {
  digitalWrite(0, MOTOR_FULL_SPEED);
  digitalWrite(4, MOTOR_STOP);
}

void forward_MD2_CH_A() {
  digitalWrite(4, MOTOR_FULL_SPEED);
  digitalWrite(0, MOTOR_STOP);
}

// motor 2 channel b
void backward_MD2_CH_B() {
  digitalWrite(5, MOTOR_FULL_SPEED);
  digitalWrite(18, MOTOR_STOP);
}
void forward_MD2_CH_B() {
  digitalWrite(18, MOTOR_FULL_SPEED);
  digitalWrite(5, MOTOR_STOP);
}


/* --------------------------------------------------- setup ---------------------------------------------------------------*/
void setup() { 
  Serial.begin(BAUD_RATE);
  // Set the motor control pins as output for Motor Driver 1
  pinMode(M1_A_IA, OUTPUT);
  pinMode(M1_A_IB, OUTPUT);
  pinMode(M1_B_IA, OUTPUT);
  pinMode(M1_B_IB, OUTPUT);
  delay(1000);
  // Set the motor control pins as output for Motor Driver 2
  pinMode(M2_A_IA, OUTPUT);
  pinMode(M2_A_IB, OUTPUT);
  pinMode(M2_B_IA, OUTPUT);
  pinMode(M2_B_IB, OUTPUT);
  delay(1000);
  reset_md_states();
}


/* ----------------------------------------------------------- loop ---------------------------------------------------------------*/
void loop() { 
  if (loop_iter % 2 == 0) {
    backward_MD1_CH_A();
    backward_MD1_CH_B();
    backward_MD2_CH_A();
    backward_MD2_CH_B();
    delay(160); // 160 [mili-sec] toggle delay
  }
  else {
    forward_MD1_CH_A();
    forward_MD1_CH_A();
    forward_MD1_CH_B();
    forward_MD2_CH_A();
    forward_MD2_CH_B();
    delay(160); // 160 [mili-sec] toggle delay
  }
  loop_iter++;
}
