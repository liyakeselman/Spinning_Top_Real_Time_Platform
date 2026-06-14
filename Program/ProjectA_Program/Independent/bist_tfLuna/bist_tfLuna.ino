/* --------------------------------------------------- include files ---------------------------------------------------*/
#include <HardwareSerial.h> // Reference the ESP32 built-in serial port library
#include <Arduino.h>

/* --------------------------------------------------- definitions ---------------------------------------------------------*/
// define baud rate
#define BAUD_RATE 115200 
// define control pins for tfLuna sensor
#define GPIO_17_TX2 17 // ESP_LUNA_LED8_TXD
#define GPIO_16_RX2 16 // LUNA_ESP_RXD

/* --------------------------------------------------- parameters ----------------------------------------------------------*/
const int HEADER = 0x59;
int dist, strength,check, uart[9];
float temprature;

/* --------------------------------------------------- objects -------------------------------------------------------------*/
HardwareSerial tfLuna(2); // Using serial port 2 (UART2)

/* --------------------------------------------------- setup ---------------------------------------------------------------*/
void setup() {
  // Initialize the Serial communication for debugging
  Serial.begin(BAUD_RATE);
  Serial.println("");
  Serial.println(" Initializing serial port...");
  delay(2000); // Wait for Serial to initialize
  while (!Serial) {
    Serial.println(" Initializing serial port FAILED, trying to connect again ...");
    delay(2000); // Wait for Serial to initialize
    return;
  }
  Serial.println(" SUCCESS for initializing Serial port");
  delay(2000);
  /* ###################### set pin modes for tfLuna ############################*/
  Serial.println("");
  Serial.println(" setting pin modes to tfLuna sensor ...");
  delay(2000);
  pinMode(GPIO_16_RX2, INPUT);
  pinMode(GPIO_17_TX2, OUTPUT);
  Serial.println(" setting pin modes to tfLuna sensor - done");
  delay(2000);
  /* ################################################### setup tfLuna #######################################################*/
  // Initialize the tfLuna communication
  Serial.println("");
  Serial.println(" setup tfLuna begin :  Initializing Serial2 port (UART2) ...");
  tfLuna.begin(BAUD_RATE, SERIAL_8N1, GPIO_16_RX2, GPIO_17_TX2); 
  delay(2000); // Wait for Serial to initialize
  while (!tfLuna) {
    Serial.println("");
    Serial.println(" Initializing Serial2 port (UART2) FAILED, trying to connect again ...");
    delay(2000); // Wait for Serial to initialize
    return;
  }
  Serial.println(" SUCCESS for initializing Serial2 port (UART2)");
  delay(2000);

  Serial.println("");
  Serial.println("");
  Serial.println(" NOTE:\n distance units for tf-luna sensor are [cm]");
  Serial.println(" distance from tf-luna sensor is unreliable when strength < 100 or strength = 65535 (overexposure)");
  delay(2000);
  Serial.println("");
  Serial.println("");
  Serial.println(" DONE : setup tfLuna");
  delay(2000);
}

/* ----------------------------------------------------------- loop ---------------------------------------------------------------*/
void loop() {
  delay (200);  
  if (tfLuna.available()) {
    if (tfLuna.read() == HEADER) {
      uart[0] = HEADER;
      if (tfLuna.read() == HEADER) {
        uart[1] = HEADER;
        for (int i =2; i < 9; ++i) {
          uart[i] = tfLuna.read();
        }
        check = uart[0] + uart[1] + uart[2] +uart[3] +uart[4] + uart[5] + uart[6] +uart[7];
        if (uart[8] == (check & 0xff)){
          dist = uart[2] + uart[3] * 256; //calculate distance value
          strength = uart[4] + uart[5] * 256; //calculate signal strength value
          temprature = uart[6] + uart[7] *256;//calculate chip temprature
          temprature = temprature/8 - 256;

           Serial.println(" distance [cm] : " 
           + String(dist) + " [cm]    strength : " 
           + String(strength) + " [..]    chip temprature : " 
           + String(temprature) + "[degC]");
           Serial.println("");
        }
      }
    }
  } else {
     Serial.println(" data is not available for reading");
  }
}

