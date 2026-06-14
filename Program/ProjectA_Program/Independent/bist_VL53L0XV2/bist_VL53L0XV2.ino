/* --------------------------------------------------- include files ---------------------------------------------------*/
#include <Wire.h>
#include "Adafruit_VL53L0X.h"

/* --------------------------------------------------- definitions ---------------------------------------------------------*/
// define baud rate
#define BAUD_RATE 115200 
// define control pins for 3 VL53L0XV2 sensors communication 
// address we will assign if triple sensor is present
#define LOX1_ADDRESS 0x21
#define LOX2_ADDRESS 0x22
#define LOX3_ADDRESS 0x23
// define control pins for 3 VL53L0XV2 sensors to shutdown
#define SHT_LOX1 13
#define SHT_LOX2 23
#define SHT_LOX3 15

/* --------------------------------------------------- objects -------------------------------------------------------------*/
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox3 = Adafruit_VL53L0X();

/* --------------------------------------------------- parameters ----------------------------------------------------------*/
VL53L0X_RangingMeasurementData_t measure1; // this holds the measurement to VL1
VL53L0X_RangingMeasurementData_t measure2; // this holds the measurement to VL2
VL53L0X_RangingMeasurementData_t measure3; // this holds the measurement to VL3

/* --------------------------------------------------- helper functions for VL53L0XV2 sensors-------------------------------------------------------*/
/* ######################  VL53L0XV2 : void setID() helper function 1 ############################*/
/*
    Reset all sensors by setting all of their XSHUT pins low for delay(10), then set all XSHUT high to bring out of reset
    Keep sensor #1 awake by keeping XSHUT pin high
    Put all other sensors into shutdown by pulling XSHUT pins low
    Initialize sensor #1 with lox.begin(new_i2c_address) Pick any number but 0x29 and it must be under 0x7F. Going with 0x30 to 0x3F is probably OK.
    Keep sensor #1 awake, and now bring sensor #2 out of reset by setting its XSHUT pin high.
    Initialize sensor #2 with lox.begin(new_i2c_address) Pick any number but 0x29 and whatever you set the first sensor to
 */
void setID() {
  // all reset
  digitalWrite(SHT_LOX1, LOW);    
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);
  delay(10);
  // all unreset
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, HIGH);
  digitalWrite(SHT_LOX3, HIGH);
  delay(10);

  // activating LOX1 and resetting LOX2 L0X3
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);

  // initing LOX1
  if(!lox1.begin(LOX1_ADDRESS)) {
    Serial.println(F("Failed to boot first VL53L0X"));
    while(1);
  }
  delay(10);

  // activating LOX2
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  //initing LOX2
  if(!lox2.begin(LOX2_ADDRESS)) {
    Serial.println(F("Failed to boot second VL53L0X"));
    while(1);
  }
  delay(10);

  // activating LOX3
  digitalWrite(SHT_LOX3, HIGH);
  delay(10);

  //initing LOX3
  if(!lox3.begin(LOX3_ADDRESS)) {
    Serial.println(F("Failed to boot third VL53L0X"));
    while(1);
  }
  delay(10);
}

/* ######################  VL53L0XV2 : void read_triple_sensors() helper function 2 ############################*/
void read_triple_sensors() {
  
  lox1.rangingTest(&measure1, false); // pass in 'true' to get debug data printout!
  lox2.rangingTest(&measure2, false); // pass in 'true' to get debug data printout!
  lox3.rangingTest(&measure3, false); // pass in 'true' to get debug data printout!

  // print sensor one reading
  Serial.print(F("1: "));
  if(measure1.RangeStatus != 4) {     // if not out of range
    Serial.print(measure1.RangeMilliMeter);
    Serial.print(F(" mm "));

  } else {
    Serial.print(F("Out of range"));
  }
  
  Serial.print(F(" "));

  // print sensor two reading
  Serial.print(F("2: "));
  if(measure2.RangeStatus != 4) {
    Serial.print(measure2.RangeMilliMeter);
    Serial.print(F(" mm "));

  } else {
    Serial.print(F("Out of range"));
  }
  
  Serial.print(F(" "));

  // print sensor three reading
  Serial.print(F("3: "));
  if(measure3.RangeStatus != 4) {     // if not out of range
    Serial.print(measure3.RangeMilliMeter);
    Serial.print(F(" mm "));
  } else {
    Serial.print(F("Out of range"));
  }
  
  Serial.println();
}

/* --------------------------------------------------- setup ---------------------------------------------------------------*/
void setup() {
  // Initialize the Serial communication for debugging
  Serial.begin(BAUD_RATE);
  Serial.println("");
  Serial.println(" Initializing serial port ...");
  delay(2000); // Wait for Serial to initialize
  while (!Serial) {
    Serial.println("");
    Serial.println(" Initializing serial port FAILED, trying to connect again ...");
    delay(2000); // Wait for Serial to initialize
    return;
  }
  Serial.println(" SUCCESS for initializing Serial port");
  delay(2000);

  Serial.println("");
  Serial.println("");
  Serial.println(" setting pin modes to 3 VL53L0XV2 sensors ...");
  delay(2000);
  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);
  pinMode(SHT_LOX3, OUTPUT);
  Serial.println(" setting pin modes to 3 VL53L0XV2 sensors - done");
  delay(2000);
  Serial.println("");
  Serial.println("");
  Serial.println(" setup VL53L0XV2 begin : Shutdown VL53L0XV2 pins (LOW) ...");
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);
  Serial.println(" all VL53L0XV2 control pins are in reset mode ...");
  Serial.println(" initializing VL53L0XV2 ..."); 
  delay(2000);
  setID();
  Serial.println("");
  Serial.println("");
  Serial.println(" DONE : setup VL53L0XV2");
  delay(2000);
}

/* ----------------------------------------------------------- loop ---------------------------------------------------------------*/
void loop() {
  read_triple_sensors();
  delay(100);
}
