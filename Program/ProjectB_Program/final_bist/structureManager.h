#ifndef STRUCTUREMANAGER_H
#define STRUCTUREMANAGER_H

#pragma once

// ================= INCLUDES ======================================================

/* include files */
#include <Arduino.h>
#include <HardwareSerial.h>  // reference the ESP32 built-in serial port library
#include <vector>
#include <Ticker.h>

// ================= GLOBAL CONSTANTS ==============================================

// Thresholds for gyro Z-axis rotation speed [rad/sec]
#define GYROZ_THRESHOLD_FOR_ROTATION_START 2.0   // [rad/sec] treshold begin of rotation - can be tuned
#define GYROZ_THRESHOLD_FOR_ROTATION_STOP 0.5    // [rad/sec] treshold stop of rotation - can be tuned
// NOTE: low treshold causes drift (noise in mpu makes snap.GyroZ*age_s to be not accurate)

// constants and definitions : sampling 
constexpr int BAUD_RATE = 115200; // define baud rate
// define sampling and buffers' properties
constexpr int DESIRED_NUM_SAMPLES = 3000; // number of total samples in simulation
// NOTE: 60[sec] for sampling simulation in 20[ms] (60*1/20[ms]=3000)
constexpr int BUFFER_SIZE = 256;  // number of samples to store in each chunk (only for .bin/.csv , not used for app)
constexpr int CSV_LINE_LEN = 256; // number of chars snprintf tries to write

/* ==============================================================================
 * Wi-Fi Settings
 * NOTE: manually set parameters accourding to your network.
 * keep the quotation marks and replace the < > with the following parameters :
 * WIFI_SSID "<the name of your network>"
 * WIFI_PASSWORD "<your wifi password>"
 * ============================================================================= */

// liya's home network
constexpr const char* WIFI_SSID = "echo_2.4_guest";
constexpr const char* WIFI_PASSWORD = "20304050";

// iddo's iphone
//constexpr const char* WIFI_SSID = "iPhone"
//constexpr const char* WIFI_PASSWORD = "AdMatay14"

// liya's iphone
//constexpr const char* WIFI_SSID = "Lia-phone"
//constexpr const char* WIFI_PASSWORD = "019019019"

// technion public network
//constexpr const char* WIFI_SSID = "TechPublic"
//constexpr const char* WIFI_PASSWORD = ""

// Wi-Fi set ssid and password
constexpr const char* ssid = WIFI_SSID;
constexpr const char* password = WIFI_PASSWORD;

// ================= EXTERNAL VARIABLES ============================================

// timing and iteration 
extern int iter;
extern unsigned long toggleTime;
extern unsigned long previousSensorsSampTime;
extern unsigned long currentSensorsSampTime;
extern unsigned long previousIterTime[BUFFER_SIZE];
extern unsigned long currentIterTime[BUFFER_SIZE];
extern unsigned long iterTime[BUFFER_SIZE];
extern unsigned long timestamps[BUFFER_SIZE];

// sensors setup and motor setup flags 
extern bool is_done_setup_leds;
extern bool is_done_setup_tfLuna;
extern bool is_done_setup_VL;
extern bool is_done_setup_mpu;
extern bool is_done_setup_motors;
extern bool is_done_setup_all;

// connected components control flags 
// NOTE : manually set these flags according to connected components, to shorten loop time ! 
extern bool is_tfLuna_connected;
extern bool is_mpu_connected;
extern bool is_all_VL_connected;
extern bool is_md1_chA_connected;
extern bool is_md1_chB_connected;
extern bool is_md2_chA_connected;
extern bool is_md2_chB_connected;
extern bool is_md1_connected;
extern bool is_md2_connected;

// shared data structure
struct Data {
  // timing
  unsigned long currentTime;  // micros (for mpu)
  unsigned long previousTime; // micros (for mpu)

  // luna
  int luna_dist;
  int luna_strength;
  float luna_temp;

  // mpu
  float mpu_gyroX,mpu_gyroY,mpu_gyroZ, mpu_tot_gyro;
  float mpu_accX, mpu_accY, mpu_accZ, mpu_tot_acc;
  float mpu_temp;
  float mpuAngle;
  
  // vl
  float vldistance[3];
  float VLsAngle;
  
  // rotation
  int spinCount;
  float fusedAngle; // for display digit after correction

  // status
  uint8_t currentTopDir;
  uint8_t currentHammerDir;
  uint8_t currentSystemMode;
  uint8_t currentTopMode;
  uint8_t currentHammerMode;
};
extern Data dataAll;

// sample count helpers
extern int bufferIndex; // TODO: delete (used only in final_bist)
extern int bufferCount; // TODO: delete (used only in final_bist)

// ================= GLOBAL BINARY PACKETS =====================================

// GLOBAL BINARY STRUCT PACKET 
#pragma pack(push, 1) // ensures there are no spaces of "garbage" between variables
struct __attribute__((__packed__)) masterPacket {
  // global data
  uint32_t iter_global;     // [0] 4 bytes: in L.E [b3:b0] // Offset=0
  uint8_t curr_activity_type;             // [1] 1 byte: [in L.E [b4] // Offset=4; 1=LUNA,2=MPU,3=VL,4=CHOICE,5=CONNECT,6=OPERATE,7=SYNC,8=MOVE_TOP ;  

  float raw_data_samp_time; // [2] 4 bytes: in L.E [b8:b5]; // Offset=5
  float binLineWr_time;     // [3] 4 bytes: [b12:b9];  // Offset=9
  float iterTime_binlineIdx;// [4] 4 bytes: [b16:b13] ; Offset=13
  
  // luna data
  int32_t luna_dist, luna_strength; // [5],[6] 4 bytes each : [b20:b17], [b24:b21]; // Offset=17,21 
  float luna_temp;    // [7] 4 bytes : [b28:b25] ; // Offset=25

  // mpu data 
  float mpuAngle, mpu_tot_gyro, mpu_tot_acc; // [8],[9],[10] 4 bytes each: [b32:b29],[b36:b33],[b40:b37] // Offset=29,33,37
  float mpu_gyroX, mpu_gyroY, mpu_gyroZ; // [11],[12],[13] 4 bytes each:[b44:b41],[b48:b45],[b52:b49] // Offset=41,45,49
  float mpu_accX, mpu_accY, mpu_accZ;    // [14],[15],[16] 4 bytes each: [b56:b53],[b60:b57],[b64:b61] //Offset=53,57,61
  
  // vl data
  float vl1_dist, vl2_dist, vl3_dist;  // [17],[18],[19] 4 bytes each:[b68:b65],[b72:b69],[b76:b73]// Offset=65,69,73
  float VLsAngle;     // [20] 4 bytes:[b80:b77] // Offset=77 

  // rotation data , 
  float fusedAngle;          // [21] 4 bytes: [b84:b81] ; Offset=81
  int32_t spinCount;          // [22] 4 bytes[b88:b85] ; Offset=85
  uint32_t bucket10;          // [23] 4 bytes [b92:b89]; Offset=89
  uint8_t digit;              // [24] 1 bytes ; [b93] Offset=93

  uint8_t currentTopDir;      // [25] 1 bytes [b94] //Offset=94 // current spinning-top direction (0-4) ; 
  uint8_t currentHammerDir;   // [26] 1 bytes[b95] // Offset=95 //current hammer direction (0-4); 
  uint8_t currentSystemMode;  // [27] 1 bytes [b96] //Offset=96 //current activity (1-8) (cmd's ID); 
  uint8_t currentTopMode;     // [28] 1 bytes [b97] // Offset=97 //current modeTop (0-1) (0 for top is not spinning [top mode is Off], and 1 for top is spinning [top mode is On]); 
  uint8_t currentHammerMode;  // [29] 1 bytes [b98]// Offset=98 //current hammerMode (0-1); 
}; 
#pragma pack(pop)
// max offset in 98 bytes (little endian,no padding,no alignment)

// GLOBAL BINARY CONTROL COMMAND STRUCT PACKET
#pragma pack(push, 1) // ensures there are no spaces of "garbage" between variables
struct __attribute__((__packed__)) ControlCommand {
    uint8_t commandId;   // 1 = moveTop,2 = battery, 3 = modeTop, 4 = changeActivity, 5 = modeHammer, 6 = moveHammer
    uint8_t direction;   // 0 = Stop, 1 = Left, 2 = Right, 3 = Forward, 4 = Backward
    uint8_t mode;        // 0 = Off, 1 = On
    uint8_t value;       // extra param if needed (for example knocking intensity
}; // 4 bytes
#pragma pack(pop)


extern masterPacket dataBuffer[BUFFER_SIZE];
extern masterPacket dataBufferWrite[BUFFER_SIZE];
extern ControlCommand command; // operate

enum ACTIVITY {LUNA = 1, MPU = 2, VL = 3, CHOICE = 4, CONNECT = 5, OPERATE = 6, SYNC = 7, MOVE_TOP = 8};
enum State { ROTATING, STOPPED };

extern ACTIVITY current_activity;
extern State currentState;

//mode button status from app
extern String Mode; 

// directions from app
extern bool is_left;
extern bool is_right;
extern bool is_forward;
extern bool is_backward;

// current states from app
extern volatile uint8_t currentTopDir;     // current spinning-top direction (0-4)
extern volatile uint8_t currentHammerDir;  // current hammer direction (0-4)
extern volatile uint8_t currentSystemMode; // current activity (1-8) // ID to cmd
extern volatile uint8_t currentTopMode;    // current top mode (0-1) (0 for top is not spinning [top mode is Off], and 1 for top is spinning [top mode is On])
extern volatile uint8_t currentHammerMode; // current hammer mode (0-1)


#endif