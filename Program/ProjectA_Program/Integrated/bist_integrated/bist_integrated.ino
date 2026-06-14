/* **********************************************************************************************************************
 * copyright owner : Keselman Liya , Electrical and Computer Engineering Faculty, Technion ------------------------------
 * ********************************************************************************************************************** */
/* **********************************************************************************************************************
 * Project A : Spinning-Top Hardware ************************************************************************************
 * Laboratory : High Speed Digital Systems Laboratory *******************************************************************
 * Faculty : Electrical and Computer Engineering, Technion **************************************************************
 * Instructor : Boaz Mizrahi ********************************************************************************************
 * Presentors : Keselman Liya and Iddo Jerushalmi ***********************************************************************
 * date : 01/2025 *******************************************************************************************************
 * ********************************************************************************************************************** */


/* ----------------------------------------------------------------------------------------------------------------------
 * include files --------------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <FS.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include <Wire.h> 
#include <Adafruit_Sensor.h>
#include "Adafruit_VL53L0X.h"
#include <Adafruit_MPU6050.h>
#include <HardwareSerial.h> // reference the ESP32 built-in serial port library
#include <VL53L0X.h>

/* ----------------------------------------------------------------------------------------------------------------------
 * macro definitions ----------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */
 
 #define FILLARRAY(a, n) for (size_t i = 0; i < (sizeof(a) / sizeof(a[0])); i++) { a[i] = n; }

/* ----------------------------------------------------------------------------------------------------------------------
 * constants and definitions --------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */
 
// #1 constants and definitions : general -------------------------------------------------------------------------------
// define constant to change to debug mode
bool debug_mode = false;
// define baud rate
const int BAUD_RATE = 115200;

// #2 constants and definitions : sampling ------------------------------------------------------------------------------
// define sampling and buffers' properties
const int DESIRED_NUM_SAMPLES = 1000; // number of total samples in simulation
const int BUFFER_SIZE = 100; // number of samples to store in each chunk
int bufferIndex = 0;
int bufferCount = 0;
String dataBuffer[BUFFER_SIZE]; // buffer to store CSV lines before writing to file

// #3 constants and definitions : leds ----------------------------------------------------------------------------------
// define control pins leds
const int LEDS_NUM = 8;
enum LEDS {LED1 = 32, LED2 = 33, LED3 = 25, LED4 = 26, LED5 = 27, LED6 = 14, LED7 = 12  , LED8 = 17};
const int gpio_led[LEDS_NUM] = {LED1, LED2, LED3, LED4, LED5, LED6 , LED7, LED8}; // GPIO pins

// #4 constants and definitions : tf-luna -------------------------------------------------------------------------------
// define control pins for tfLuna sensor communication
const int GPIO_17_TX2 = 17; // ESP_LUNA_LED8_TXD
const int GPIO_16_RX2 = 16; // LUNA_ESP_RXD
const int HEADER = 0x59;    // constant for tf-luna operation

// #5 constants and definitions : VL53L0XV2 -----------------------------------------------------------------------------
// define control pins for 3 VL53L0XV2 sensors communication 
const int LOX1_ADDRESS = 0x21; 
const int LOX2_ADDRESS = 0x22; 
const int LOX3_ADDRESS = 0x23;
// define control pins for 3 VL53L0XV2 sensors to shutdown
const int SHT_LOX1 = 13;
const int SHT_LOX2 = 23;
const int SHT_LOX3 = 15;

// #6 constants and definitions : motor-drivers -------------------------------------------------------------------------
// define average motor phase time
const unsigned long AVG_MOTOR_PHASE_TIME = 7;
// define motors half_period
const double MOTOR_HALF_PERIOD = 50;
// define control pins for motor driver 1
const int M1_A_IA = 2;
const int M1_A_IB = 1;
const int M1_B_IA = 19;
const int M1_B_IB = 3; 
// define control pins for motor driver 2
const int M2_A_IA = 0;
const int M2_A_IB = 4;
const int M2_B_IA = 5;
const int M2_B_IB = 18; 

// #7 constants and definitions : network -------------------------------------------------------------------------------
/* NOTE: manually set parameters accourding to your network.
 * keep the quotation marks and replace the < > with the following parameters :
 * WIFI_SSID "<the name of your network>"
 * WIFI_PASSWORD "<your wifi password>"
*/

// liya's dormitories network
//#define WIFI_SSID "HOTBOX-6B4F"
//#define WIFI_PASSWORD "10203040"

// iddo's iphone
//#define WIFI_SSID "iPhone"
//#define WIFI_PASSWORD "AdMatay14"

// liya's iphone
//#define WIFI_SSID "Lia-phone"
//#define WIFI_PASSWORD "019019019"

#define WIFI_SSID "TechPublic"
#define WIFI_PASSWORD ""

const char* ssid = WIFI_SSID; 
const char* password = WIFI_PASSWORD;

// ------------------------------- file constants and defenitions ----------------------------------------------------------
#define FILE_NAME "/sensors_data.csv"

/* ----------------------------------------------------------------------------------------------------------------------
 * global variables -----------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */
 
// #1 global variables : timing and iteration ---------------------------------------------------------------------------
int iter                                       = 0;
unsigned long toggleTime                       = 0;
unsigned long previousSensorsSampTime          = 0;
unsigned long currentSensorsSampTime           = 0;
unsigned long sampTimestamp                    = 0;
unsigned long previousIterTime[BUFFER_SIZE]    = {0};
unsigned long currentIterTime[BUFFER_SIZE]     = {0};
unsigned long iterTime[BUFFER_SIZE]            = {0}; 
unsigned long timestamps[BUFFER_SIZE]          = {0};
unsigned long motors_phase_time[BUFFER_SIZE]   = {0};
unsigned long loop_motor_begin[BUFFER_SIZE]    = {0};
unsigned long loop_motor_end[BUFFER_SIZE]      = {0};

// #2 global variables : handling file variables ------------------------------------------------------------------------
String csvDataHeader = 
                        String("iteration index, ")                +  
                        String("timestamp [ms], ")                 +  
                        String("iterTime [ms], ")                  + // for debug
                        String("toggleTime [ms], ")                + // for debug
                        String("motors_phase_time[iter], ")        + // for debug

                        String("tfLuna distance [cm], ")           +  
                        String("tfLuna strength, ")                +  
                        String("tfLuna chip temperature [degC], ") +  
                        String("mpu acceleration X [m/s^2], ")     +  
                        String("mpu acceleration Y [m/s^2], ")     +  
                        String("mpu acceleration Z [m/s^2], ")     +  
                        String("mpu rotation X [rad/s], ")         +  
                        String("mpu rotation Y (rad/s), ")         +  
                        String("mpu rotation Z [rad/s], ")         +  
                        String("mpu chip temprature [degC], ")     +  
                        String("VL53L0X 1 distance [mm], ")        +  
                        String("VL53L0X 2 distance [mm], ")        +  
                        String("VL53L0X 3 distance [mm]\n");

// #3 global variables :  sensors setup and motor setup flags -----------------------------------------------------------
bool is_done_setup_leds   = false;
bool is_done_setup_tfLuna = false;
bool is_done_setup_VL     = false;
bool is_done_setup_mpu    = false;
bool is_done_setup_motors = false;
bool is_done_setup_all    = false;

// #4 global variables : connected components control flags ------------------------------------------------------------- 
/* NOTE : manually set these flags according to connected components, to shorten loop time ! */
bool is_tfLuna_connected  = false;
bool is_mpu_connected     = true;
bool is_all_VL_connected  = true;
bool is_md1_chA_connected = false; 
bool is_md1_chB_connected = false;
bool is_md2_chA_connected = false;
bool is_md2_chB_connected = true; 
bool is_md1_connected     = is_md1_chA_connected || is_md1_chB_connected; 
bool is_md2_connected     = is_md2_chA_connected || is_md2_chB_connected;

// #5 global variables : tf-luna operation ------------------------------------------------------------------------------
int dist, strength,check, uart[9];
float temprature;
bool canReadFromTFLuna = false;             // a flag to see if data is available for reading from tfLuna sensor
int tfLuna_distances[BUFFER_SIZE];          // [cm] (stores tf-luna distance data in a buffer)
int tfLuna_strengths[BUFFER_SIZE];          // [..] (stores tf-luna strengths data in a buffer)
float tfLuna_chip_tempratures[BUFFER_SIZE]; // [degC] (stores tf-luna chip tempratures data in a buffer)

// #6 global variables : mpu6050 operation variables --------------------------------------------------------------------
float mpu_acceleration_x[BUFFER_SIZE];      // [m/s^2] (stores mpu acceleration data in x axis in a buffer)
float mpu_acceleration_y[BUFFER_SIZE];      // [m/s^2] (stores mpu acceleration data in y axis in a buffer)
float mpu_acceleration_z[BUFFER_SIZE];      // [m/s^2] (stores mpu acceleration data in z axis in a buffer)
float mpu_rotation_x[BUFFER_SIZE];          // [rad/s] (stores mpu rotation data in x axis in a buffer)
float mpu_rotation_y[BUFFER_SIZE];          // [rad/s] (stores mpu rotation data in y axis in a buffer)
float mpu_rotation_z[BUFFER_SIZE];          // [rad/s] (stores mpu rotation data in z axis in a buffer)
float mpu_chip_temperature[BUFFER_SIZE];    // [degC]  (stores mpu chip temperature data in a buffer)

// #7 global variables : VL53L0XV2 operation variables and flags --------------------------------------------------------
uint16_t measure1_reading;                    // hold a measurement for lox1 sensor
uint16_t measure2_reading;                    // hold a measurement for lox2 sensor
uint16_t measure3_reading;                    // hold a measurement for lox3 sensor
bool done_to_boot_L0X1 = false;               // hold boot state for lox1 sensor
bool done_to_boot_L0X2 = false;               // hold boot state for lox2 sensor
bool done_to_boot_L0X3 = false;               // hold boot state for lox3 sensor
uint16_t VL1_measures[BUFFER_SIZE];           // (stores lox1 distance data in a buffer)
uint16_t VL2_measures[BUFFER_SIZE];           // (stores lox2 distance data in a buffer)
uint16_t VL3_measures[BUFFER_SIZE];           // (stores lox3 distance data in a buffer)

/* ----------------------------------------------------------------------------------------------------------------------
 * global objects -------------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */
 
// #1 global objects : sensors objects ----------------------------------------------------------------------------------
HardwareSerial tfLuna(2); // using serial port 2 (UART2)
VL53L0X lox1 = VL53L0X(); // I2C
VL53L0X lox2 = VL53L0X(); // I2C
VL53L0X lox3 = VL53L0X(); // I2C
Adafruit_MPU6050 mpu;     // I2C

// #2 global objects : VL53L0XV2 measurment data holder objects ---------------------------------------------------------
VL53L0X_RangingMeasurementData_t measure1; // [mm]
VL53L0X_RangingMeasurementData_t measure2; // [mm]
VL53L0X_RangingMeasurementData_t measure3; // [mm]

// #3 global objects : web server and file handling objects -------------------------------------------------------------
AsyncWebServer server(80); // create an instance of the web server (HTTP server)
File file;

/* ----------------------------------------------------------------------------------------------------------------------
 * function declarations ------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */
 
// #1 function declarations : initialization functions ------------------------------------------------------------------
void serialInit();
void SPIFFSinit();
void wifiInit();
void initializeWebServer();

// #2 function declarations : introduction to user ----------------------------------------------------------------------
void wellcomeMessage();
void WarningsToUser();

// #3 function declarations : setup helper functions --------------------------------------------------------------------
void setupLeds();
void setupTFluna();
void setID();
void setup3VL();
void setupMPU();
void setupMotorDrivers();

// #4 function declarations : shutting down helper functions ------------------------------------------------------------
void shutdownTFluna();
void shutdown3VL();
void shutdownMPU();
void shutdownMotors();

// #5 function declarations :  call helper functions --------------------------------------------------------------------
void callInitTest();
void callSetupSensors();
void callShutdownComponents();

// #6 function declarations :  handle iteration helper functions --------------------------------------------------------
void handleLunaIter();
void handleMPUIter();
void handle3VLIter();

// #7 function declarations : leds control functions --------------------------------------------------------------------
void turnOnLeds();
void turnOffLeds();

// #8 function declarations :  handle motor-driver movement helper functions --------------------------------------------
void backward();
void forward();

// #9 function declarations : handle file functions ---------------------------------------------------------------------
String returnCSVline();
void openCSVFileToAppend();
void prepareCSVFile();
void reset_buffers();
void writeDataToFile();
void notFound(AsyncWebServerRequest *request);

/* ----------------------------------------------------------------------------------------------------------------------
 * main setup -----------------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */
 
void setup() {
  bufferIndex = 0;
  callInitTest();
  wellcomeMessage();
  callSetupSensors();
  WarningsToUser();  //  presents next operations

  if (!debug_mode) {
    // (1) next operation : shutting down Serial port
    Serial.end(); 
    delay(2000);
    // (2)  next operation : set pin modes to motor-drivers + (3) next operation : reset control pins of motor-drivers to LOW
    setupMotorDrivers(); 
    if (is_done_setup_leds && is_done_setup_tfLuna && is_done_setup_VL && is_done_setup_mpu && is_done_setup_motors) {
      is_done_setup_all = true;
    }
  }
  else { 
       // Serial port must be on and all motor-drivers codes should be in comment to debug
       if (is_done_setup_leds && is_done_setup_tfLuna && is_done_setup_VL && is_done_setup_mpu) {
          is_done_setup_all = true;
          Serial.println("complete to setup all sensors");
      }
  }
  
  prepareCSVFile();

  previousIterTime[0] = millis();
  previousSensorsSampTime = millis();

  // (4) next operation : start loop iterations

}

/* ----------------------------------------------------------------------------------------------------------------------
 * main loop ------------------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */
 
void loop() {
  //Serial.println("iteration number : " + String(iter)); //for debugging (Serial port must be on and all motor-drivers codes should be in comment)
  if (is_done_setup_all) {
    if (iter == BUFFER_SIZE) {
        writeDataToFile();
        reset_buffers();
        bufferIndex ++;
        iter = 0;
    }
    if (bufferIndex * BUFFER_SIZE + iter == DESIRED_NUM_SAMPLES) {
      serialInit();
      Serial.println(" done with " + String(DESIRED_NUM_SAMPLES) + " iterations");
      Serial.println(" open your browser and navigate to http://" + WiFi.localIP().toString() + " to download the csv file.");
      Serial.println("");
      Serial.println("");
      Serial.println(" program effectively ended !");
      delay(1000);
      callShutdownComponents();
      file.close(); 
      delay(1000);
      while(1);
    }

    if (is_tfLuna_connected) handleLunaIter(); 
    if(is_all_VL_connected) handle3VLIter();
    if (is_mpu_connected) handleMPUIter();
    
    currentSensorsSampTime = millis();
    sampTimestamp += currentSensorsSampTime - previousSensorsSampTime;
    previousSensorsSampTime = currentSensorsSampTime;
    timestamps[iter] = sampTimestamp;  // store timestamp 

    loop_motor_end[iter] = millis();
    // calculate motors phase time
    motors_phase_time[iter] = iter > 0 ? loop_motor_end[iter] - loop_motor_begin[iter - 1] : AVG_MOTOR_PHASE_TIME;
    // calculate toggleTime
    toggleTime = max(0.0, MOTOR_HALF_PERIOD-motors_phase_time[iter]);
    // move motors
    if (iter % 2 == 0) { // move motor backward in even iterations and turn on leds
      backward();
      delay(toggleTime);
      turnOnLeds();
    }
    else {  // move motor forward in off iterations and turn on leds
      forward();
      delay(toggleTime);
      turnOffLeds();
    }
    loop_motor_begin[iter] = millis();

    currentIterTime[iter] = millis();
    iterTime[iter] = currentIterTime[iter] - previousIterTime[iter];

    // store csv line in file in real time
    dataBuffer[bufferCount++] = returnCSVline();

    /* // print csv line to file in real time
    *  file.println(returnCSVline());
    */
    
    // if buffer is full, write to file to avoid overflow
    if (bufferCount == BUFFER_SIZE) {
      writeDataToFile();
    }

    iter++;
    previousIterTime[iter] = currentIterTime[iter-1];

  // handle setup error
  } else {
      serialInit();
      Serial.println(" ERROR : setup error (is_done_setup_all = false)");
      Serial.println("");
      Serial.println("");
      Serial.println(" program effectively ended !");
      delay(1000);
      callShutdownComponents();
      file.close(); 
      delay(1000);
      while(1);
  }
}

/* ----------------------------------------------------------------------------------------------------------------------
 * initialization functions ---------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */

// #1 initialization function : inits Serial port -----------------------------------------------------------------------
void serialInit() {
  Serial.begin(BAUD_RATE);
  Serial.println("");
  Serial.println(" Initializing serial port ...");
  delay(2000); // Wait for Serial to initialize
  while (!Serial) {
    Serial.println(" ERROR : Initializing serial port FAILED, Retrying ...");
    delay(2000); // Wait for Serial to initialize
    return;
  }
  Serial.println(" Serial port initialized successfully");
  delay(1000);
}

// #2 initialization function : inits SPIFFS ----------------------------------------------------------------------------
void SPIFFSinit() {
  // Initialize SPIFFS
  Serial.println("");
  Serial.println(" Initializing SPIFFS ...");
  if (!SPIFFS.begin(true)) {
    Serial.println(" ERROR : Failed to mount file system");
    return;
  }
  Serial.println(" SPIFFS mounted successfully");
}

// #3 initialization function : inits Wi-Fi conncection -----------------------------------------------------------------
void wifiInit() {
  // Initialize WiFi connection
  Serial.println("");
  Serial.println(" Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println(" ERROR : WiFi connection failed");
      return;
  }
  Serial.println(" Connected to WiFi");
  Serial.println(" IP Address: " + WiFi.localIP().toString());
  delay(2000); 
}

// #4 initialization functions : inits web server conncetion ------------------------------------------------------------
void initializeWebServer() {
  // Serve the CSV file
  server.on(FILE_NAME, HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, FILE_NAME, "text/csv");
  });

  // Serve the file from SPIFFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (SPIFFS.exists(FILE_NAME)) {
      File file = SPIFFS.open(FILE_NAME, "r");
      request->send(file, "text/plain");
    } else {
      request->send(404, "text/plain", "File not found");
    }
  });

  // Start the server
  server.begin(); 
  Serial.println(" HTTP server started"); 
}

/* ----------------------------------------------------------------------------------------------------------------------
 * introduction to user functions ---------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */

// #1 introduction to user function :introduces the test ----------------------------------------------------------------
void wellcomeMessage() {
  Serial.println("");
  Serial.println(" This is a test to check the spinning top components ...");
  delay(1000); 
  Serial.println("");
  Serial.println(" Preparing to setup units ...");
  delay(1000); 
}

// #2 introduction to user function : introduces warnings and next operation --------------------------------------------
void WarningsToUser() {
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println(" ###### WARNINGS ###### ");
  Serial.println(" (1) we will shut down the Serial port");
  delay(1000);
  Serial.println(" NOTE !!!" );
  Serial.println(" Serial port will be off, so printings to Serial Monitor will not be available !" );
  delay(1000);
  Serial.println(" next operations: ");
  Serial.println(" (2) we will set pin modes to motor-drivers");
  Serial.println(" (3) we will reset control pins of motor-drivers to LOW");
  Serial.println(" (4) we will start loop iterations");
  delay(1000);
}

/* ----------------------------------------------------------------------------------------------------------------------
 * setup helper functions -----------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */

// #1 setup helper function : setup leds --------------------------------------------------------------------------------
void setupLeds() {
  // set pinmodes for leds
  Serial.println("");
  Serial.println(" setting pin modes to leds ...");
  for (int i = 0; i < LEDS_NUM; ++i) {
    pinMode(gpio_led[i], OUTPUT);
    delay(10);
  }
  delay(1000);
  Serial.println(" setting pin modes to leds - done");
  delay(1000);
  is_done_setup_leds = true;
  Serial.println("");
  Serial.println("");
  Serial.println(" DONE : setup leds");
}

// #2 setup helper function : setup tf-luna -----------------------------------------------------------------------------
void setupTFluna() {
  // set pinmodes for tfLuna
  Serial.println("");
  Serial.println(" setting pin modes to tfLuna sensor ...");
  delay(1000);
  pinMode(GPIO_16_RX2, INPUT);  delay(10);
  pinMode(GPIO_17_TX2, OUTPUT); delay(10);
  Serial.println(" setting pin modes to tfLuna sensor - done");
  delay(1000);
  // setup tf-luna (initialize the tfLuna communication)
  Serial.println("");
  Serial.println(" setup tfLuna begin :  Initializing Serial2 port (UART2) ...");
  tfLuna.begin(BAUD_RATE, SERIAL_8N1, GPIO_16_RX2, GPIO_17_TX2); 
  delay(2000); // wait for Serial to initialize
  while (!tfLuna) {
    Serial.println("");
    Serial.println(" ERROR : Initializing Serial2 port (UART2) FAILED, trying to connect again ...");
    delay(2000); // wait for Serial to initialize
    return;
  }
  Serial.println(" SUCCESS for initializing Serial2 port (UART2)");
  delay(1000);

  Serial.println("");
  Serial.println("");
  Serial.println(" NOTE:\n distance units for tf-luna sensor are [cm]");
  Serial.println(" distance from tf-luna sensor is unreliable when strength < 100 or strength = 65535 (overexposure)");
  delay(1000);

  Serial.println("");
  Serial.println(" Initiates data array of tfLuna to invalid values ...");
  delay(1000);

  FILLARRAY(tfLuna_distances,-1);
  FILLARRAY(tfLuna_strengths,-1);
  FILLARRAY(tfLuna_chip_tempratures,-1.0);

  delay(1000);
  Serial.println(" SUCCESS for initializing data array of tfLuna to invalid values");
  delay(1000);
  Serial.println("");
  Serial.println("");
  Serial.println(" DONE : setup tfLuna");
  delay(1000);
  is_done_setup_tfLuna = true;
}

// #3 setup helper function : setID for 3 VL53L0XV2 ---------------------------------------------------------------------
void setID() {
  // reset all sensors
  digitalWrite(SHT_LOX1, LOW);    
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);
  delay(10);
  
  Wire.begin();
  // bring up sensors one by one and set their addresses
  digitalWrite(SHT_LOX1, HIGH); delay(10);
  lox1.init(); 
  if (!lox1.init()) {
    Serial.println("ERROR : Failed to initialize LOX1");
    while (1);
  }
  lox1.setAddress(LOX1_ADDRESS); delay (10);
  digitalWrite(SHT_LOX2, HIGH); delay(10);
  lox2.init(); 
  if (!lox2.init()) {
    Serial.println("ERROR : Failed to initialize LOX2");
    while (1);
  }
  lox2.setAddress(LOX2_ADDRESS); delay (10);
  digitalWrite(SHT_LOX3, HIGH); delay(10);
  lox3.init(); 
  if (!lox3.init()) {
    Serial.println("ERROR : Failed to initialize LOX3");
    while (1);
  }
  lox3.setAddress(LOX3_ADDRESS); delay (10);
  
  // set measurement timing budget
  lox1.setMeasurementTimingBudget(20000); // 20 [ms]
  lox2.setMeasurementTimingBudget(20000); // 20 [ms]
  lox3.setMeasurementTimingBudget(20000); // 20 [ms] 
  
  // start continuous mode for all sensors
  lox1.startContinuous();
  lox2.startContinuous();
  lox3.startContinuous();

  // mark sensors as booted 
  done_to_boot_L0X1 = true;
  done_to_boot_L0X2 = true;
  done_to_boot_L0X3 = true;
}

// #4 setup helper function : setup 3 VL53L0XV2 -------------------------------------------------------------------------
void setup3VL() {
  // set pinmodes for VL53L0XV2
  Serial.println("");
  Serial.println("");
  Serial.println(" setting pin modes to 3 VL53L0XV2 sensors ...");
  delay(1000);
  pinMode(SHT_LOX1, OUTPUT); delay(10);
  pinMode(SHT_LOX2, OUTPUT); delay(10);
  pinMode(SHT_LOX3, OUTPUT); delay(10);
  Serial.println(" setting pin modes to 3 VL53L0XV2 sensors - done");
  delay(1000);
  // setup VL53L0XV2
  Serial.println("");
  Serial.println("");
  Serial.println(" setup VL53L0XV2 begin : Shutdown VL53L0XV2 pins (LOW) ...");
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);
  delay(10);
  Serial.println(" all VL53L0XV2 control pins are in reset mode ...");
  Serial.println(" initializing VL53L0XV2 ..."); 
  setID();
  // check if sensors are initialized
  if (!done_to_boot_L0X1 || !done_to_boot_L0X2 || !done_to_boot_L0X3) {
    Serial.println(" ERROR : failed to initialize one or more VL53L0X sensors");
    while (1); 
  }
  Serial.println("");
  Serial.println(" Initiates data arrays of VL53L0XV2 to invalid values ...");
  VL1_measures[BUFFER_SIZE] = {0};
  VL2_measures[BUFFER_SIZE] = {0};
  VL3_measures[BUFFER_SIZE] = {0};
  delay(1000);
  Serial.println(" SUCCESS for initializing data arrays of VL53L0XV2 to invalid values");
  delay(1000);
  Serial.println("");
  Serial.println("");
  Serial.println(" DONE : setup VL53L0XV2");
  delay(1000);
  is_done_setup_VL = true;
}

// #5 setup helper function : setup mpu6050 -----------------------------------------------------------------------------
void setupMPU() {
  // setup mpu6050
  Serial.println("");
  Serial.println("");
  Serial.println(" setup MPU6050 begin :");
  delay(1000);
  // try to initialize 
  if (!mpu.begin()) {
    Serial.println("");
    Serial.println(" ERROR : failed to find MPU6050 chip");
      delay(10);
  }
  Serial.println(" MPU6050 Found!");
  delay(1000);

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print(" Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println(" +-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println(" +-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println(" +-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println(" +-16G");
    break;
  }
  
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print(" Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println(" +- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println(" +- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println(" +- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println(" +- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print(" Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println(" 260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println(" 184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println(" 94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println(" 44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println(" 21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println(" 10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println(" 5 Hz");
    break;
  }

  Serial.println("");
  Serial.println(" Initiates data arrays of mpu to invalid values ...");

  FILLARRAY(mpu_acceleration_x, -1.0);
  FILLARRAY(mpu_acceleration_y, -1.0);
  FILLARRAY(mpu_acceleration_z, -1.0);
  FILLARRAY(mpu_rotation_x, -1.0);
  FILLARRAY(mpu_rotation_y, -1.0);
  FILLARRAY(mpu_rotation_z, -1.0);
  FILLARRAY(mpu_chip_temperature, -1.0);

  delay(1000);
  Serial.println(" SUCCESS for initializing data arrays of mpu to invalid values");
  delay(1000);
  Serial.println("");
  Serial.println("");
  Serial.println(" DONE : setup MPU6050");
  delay(1000);
  is_done_setup_mpu = true;
}

// #6 setup helper function : setup motor-drivers -----------------------------------------------------------------------
void setupMotorDrivers() {
  // set pin modes for motor-drivers 
   if (is_md1_connected) {
  // set pinmodes for motor driver 1 
    pinMode(M1_A_IA, OUTPUT); 
    pinMode(M1_A_IB, OUTPUT);
    pinMode(M1_B_IA, OUTPUT); 
    pinMode(M1_B_IB, OUTPUT); 
  // setup motor 1 : reset control pins of motor-drivers to LOW
    digitalWrite(M1_A_IA, LOW);
    digitalWrite(M1_A_IB, LOW);
    digitalWrite(M1_B_IA, LOW);
    digitalWrite(M1_B_IB, LOW);
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

/* ----------------------------------------------------------------------------------------------------------------------
 * shutting down helper functions ---------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */

// #1  shutting down helper function : shuts down tf-luna ---------------------------------------------------------------
void shutdownTFluna() {
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);
  tfLuna.end();
}

// #2 shutting helper function : shuts down 3 VLs -----------------------------------------------------------------------
void shutdown3VL() {
  lox3.stopContinuous();
  lox2.stopContinuous();
  lox1.stopContinuous();
  digitalWrite(SHT_LOX3, LOW);
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX1, LOW);
}

// #3 shutting helper function : shuts down mpu6050 ---------------------------------------------------------------------
void shutdownMPU() {
  mpu.reset();
}

// #4 shutting helper function : shuts down motor-drivers ---------------------------------------------------------------
void shutdownMotors() {
    digitalWrite(M1_A_IA, LOW);
    digitalWrite(M1_A_IB, LOW);
    digitalWrite(M1_B_IA, LOW);
    digitalWrite(M1_B_IB, LOW);  
    digitalWrite(M2_A_IA, LOW);
    digitalWrite(M2_A_IB, LOW);
    digitalWrite(M2_B_IA, LOW);
    digitalWrite(M2_B_IB, LOW);  
}
/* ----------------------------------------------------------------------------------------------------------------------
 * call helper functions ------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */

// #1 call helper function : calls initialization functions -------------------------------------------------------------
void callInitTest() {
  serialInit();
  SPIFFSinit(); 
  wifiInit();  
  initializeWebServer();
}

// #2 call helper function : calls setup function to each sensor --------------------------------------------------------
void callSetupSensors() {
  setupLeds();
  setupTFluna();
  setup3VL();
  setupMPU();
}

// #3 call helper function : calls shutdown function to each component --------------------------------------------------
void callShutdownComponents() {
  turnOffLeds();
  shutdownTFluna();
  shutdown3VL();
  shutdownMPU();
  shutdownMotors();
}

/* ----------------------------------------------------------------------------------------------------------------------
 * handle iteration helper functions ------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */

// #1 handle iteration helper function : tf luna ------------------------------------------------------------------------
void handleLunaIter() {
  // Check if data is available from the TF-Luna sensor
  if (tfLuna.available()) {
    canReadFromTFLuna = true;
    if (tfLuna.read() == HEADER) {
      uart[0] = HEADER;
      if (tfLuna.read() == HEADER) {
        uart[1] = HEADER;
        for (int i =2; i < 9; i++) {
          uart[i] = tfLuna.read();
        }
        check = uart[0] + uart[1] + uart[2] +uart[3] +uart[4] + uart[5] + uart[6] +uart[7];
        if (uart[8] == (check & 0xff)){
          dist = uart[2] + uart[3] * 256; //calculate distance value
          strength = uart[4] + uart[5] * 256; //calculate signal strength value
          temprature = uart[6] + uart[7] *256;//calculate chip temprature
          temprature = temprature/8 - 256;
          // store samples in arrays
          tfLuna_distances[iter] = dist;
          tfLuna_strengths[iter] = strength;
          tfLuna_chip_tempratures[iter] = temprature;
        }
      }
    }
  } else {
      canReadFromTFLuna = false;
  }
}

// #2 handle iteration helper function : MPU6050 ------------------------------------------------------------------------
void handleMPUIter() {
 /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  // store samples in arrays
  mpu_acceleration_x[iter] = a.acceleration.x; // [m/s^2]
  mpu_acceleration_y[iter] = a.acceleration.y; // [m/s^2]
  mpu_acceleration_z[iter] = a.acceleration.z; // [m/s^2]
  mpu_rotation_x[iter] = g.gyro.x; // [rad/s]
  mpu_rotation_y[iter] = g.gyro.y; // [rad/s]
  mpu_rotation_z[iter] = g.gyro.z; // [rad/s]
  mpu_chip_temperature[iter] = temp.temperature; // [degC]
}

// #3 handle iteration helper function : 3 VL53L0XV2 --------------------------------------------------------------------
void handle3VLIter() {
  measure1_reading = lox1.readRangeContinuousMillimeters();
  measure2_reading = lox2.readRangeContinuousMillimeters();
  measure3_reading = lox3.readRangeContinuousMillimeters();

  VL1_measures[iter] = measure1_reading; // [mm]
  VL2_measures[iter] = measure2_reading; // [mm]
  VL3_measures[iter] = measure3_reading; // [mm]
}

/* ----------------------------------------------------------------------------------------------------------------------
 * leds control functions -----------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */

// #1 : leds control function : turn on leds ----------------------------------------------------------------------------
void turnOnLeds() {
  for (int i = 0; i < LEDS_NUM; ++i){
    digitalWrite(gpio_led[i], LOW);
  }
}

// #2 : leds control function : turn off leds ---------------------------------------------------------------------------
void turnOffLeds() {
  for (int i = 0; i < LEDS_NUM; ++i){
    digitalWrite(gpio_led[i], HIGH);
  }
}

/* ----------------------------------------------------------------------------------------------------------------------
 * handle motor-driver movement helper functions ------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */

// #1 handle motor-driver movement helper function : move backward ------------------------------------------------------
void backward() {
// TODO: this implementation is not full. we need to do it for both channels and for both motors !
  if (is_md2_connected) {
      if (is_md2_chB_connected) {
        // analogWrite(M2_B_IA, MOTOR_FULL_SPEED);
        // analogWrite(M2_B_IB, MOTOR_STOP);
        digitalWrite(M2_B_IA,HIGH);
        digitalWrite(M2_B_IB,LOW);
      }
    }
}

// #2 handle motor-driver movement helper function : move forward -------------------------------------------------------
void forward() {
  // TODO: this implementation is not full. we need to do it for both channels and for both motors !
  if (is_md2_connected) {
      if (is_md2_chB_connected) {
        // analogWrite(M2_B_IA, MOTOR_STOP);
        // analogWrite(M2_B_IB, MOTOR_FULL_SPEED);
        digitalWrite(M2_B_IA,LOW);
        digitalWrite(M2_B_IB,HIGH);
      }
   }
}

/* ----------------------------------------------------------------------------------------------------------------------
 * handle file functions ------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------------- */

// #1 handle file function : generate CSV vector (stores data values for csv file) --------------------------------------
String returnCSVline() {
  String csvLine = 
            String(bufferIndex * BUFFER_SIZE + iter) + "," +
            String(timestamps[iter]) + ","              +
            String(iterTime[iter]) + ","                + // for debug
            String(toggleTime) + ","                    + // for debug
            String(motors_phase_time[iter]) + ","       + // for debug
            String(tfLuna_distances[iter]) + ","        +
            String(tfLuna_strengths[iter]) + ","        + 
            String(tfLuna_chip_tempratures[iter]) + "," + 
            String(mpu_acceleration_x[iter]) + ","      + 
            String(mpu_acceleration_y[iter]) + ","      +
            String(mpu_acceleration_z[iter]) + ","      +
            String(mpu_rotation_x[iter]) + ","          +
            String(mpu_rotation_y[iter]) + ","          +
            String(mpu_rotation_z[iter]) + ","          +
            String(mpu_chip_temperature[iter]) + ","    +
            String(VL1_measures[iter]) + ","            +
            String(VL2_measures[iter]) + ","            +
            String(VL3_measures[iter]) + "\n";
  return csvLine;
}

// #2 handle file function : open CSV file for append -------------------------------------------------------------------
void openCSVFileToAppend() {
    file = SPIFFS.open(FILE_NAME, FILE_APPEND);
  // handle failure
  if (!file) {
    Serial.println(" ERROR : failed to open file for appending writes");
    delay(2000);
    return; // NOTE: return does not exit from loop, but continues to next iteration
  }
}

// #3 handle file function : prepares CSV file --------------------------------------------------------------------------
void prepareCSVFile() {
  // write the CSV header line to the file
  file = SPIFFS.open(FILE_NAME, FILE_WRITE);
  if (!file) {
    Serial.println(" ERROR : failed to open file for writing");
    delay(2000);
    return; // NOTE: return does not exit from loop, but continues to next iteration
  }
  
  file.println(csvDataHeader);
  // close the file
  file.close();

  openCSVFileToAppend();
}

// #4 handle file function : resets buffers for sensors -----------------------------------------------------------------
void reset_buffers() {
  // tf luna
  FILLARRAY(tfLuna_distances,-1);
  FILLARRAY(tfLuna_strengths,-1);
  FILLARRAY(tfLuna_chip_tempratures,-1.0);

  // mpu6050
  FILLARRAY(mpu_acceleration_x, -1.0);
  FILLARRAY(mpu_acceleration_y, -1.0);
  FILLARRAY(mpu_acceleration_z, -1.0);
  FILLARRAY(mpu_rotation_x, -1.0);
  FILLARRAY(mpu_rotation_y, -1.0);
  FILLARRAY(mpu_rotation_z, -1.0);
  FILLARRAY(mpu_chip_temperature, -1.0);
  
  // VL53L0XV2
  VL1_measures[BUFFER_SIZE] = {0};
  VL2_measures[BUFFER_SIZE] = {0};
  VL3_measures[BUFFER_SIZE] = {0};

  // motor drivers
  loop_motor_begin[BUFFER_SIZE] = {0};
  loop_motor_end[BUFFER_SIZE] = {0};
  motors_phase_time[BUFFER_SIZE] = {0};
  previousIterTime[0] = millis();
}

// #5 handle file function : write the stored buffer data to csv file ---------------------------------------------------
void writeDataToFile() {
  if (bufferCount == 0) return; // nothing to write
  
  openCSVFileToAppend();
  for (int i = 0 ; i < bufferCount; ++i) {
    file.println(dataBuffer[i]);
  }
  // clear the buffer
  bufferCount = 0;
  // close the file to ensure data is written
  file.close();
}

// #6 handle file function : request to web server not found ------------------------------------------------------------
void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}