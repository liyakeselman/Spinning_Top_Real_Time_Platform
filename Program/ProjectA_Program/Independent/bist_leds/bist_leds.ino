/* --------------------------------------------------- include files ---------------------------------------------------*/
#include <Arduino.h>

/* --------------------------------------------------- definitions ---------------------------------------------------------*/
// define baud rate
#define BAUD_RATE 115200 
// define control pins leds
#define LEDS_NUM 8
enum LEDS {LED1 = 32, LED2 = 33, LED3 = 25, LED4 = 26, LED5 = 27, LED6 = 14, LED7 = 12  , LED8 = 17};

/* --------------------------------------------------- parameters ----------------------------------------------------------*/
// set parameters for leds
int gpio_led[LEDS_NUM] = {LED1, LED2, LED3, LED4, LED5, LED6 , LED7, LED8};

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

  // set pinmodes for leds
  Serial.println("");
  Serial.println(" setting pin modes to leds ...");
  for (int i = 0; i < LEDS_NUM; ++i) {
    pinMode(gpio_led[i], OUTPUT);
    delay(10);
  }
  delay(2000);
  Serial.println(" setting pin modes to leds - done");
  delay(2000);
}

/* ----------------------------------------------------------- loop ---------------------------------------------------------------*/
void loop() {
  for (int i = 0; i < LEDS_NUM; ++i) {
    digitalWrite(gpio_led[i], LOW);
    Serial.printf("LED%d is on\n", i+1);
    delay(1000);
    digitalWrite(gpio_led[i], HIGH);
    Serial.printf("LED%d is off\n", i+1);
    delay(1000);
  }
}
