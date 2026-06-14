/* --------------------------------------------------- include files ---------------------------------------------------*/
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

/* --------------------------------------------------- definitions ---------------------------------------------------------*/
// define baud rate
#define BAUD_RATE 115200

/* --------------------------------------------------- objects -------------------------------------------------------------*/
Adafruit_MPU6050 mpu;

/* --------------------------------------------------- setup ---------------------------------------------------------------*/
void setup(void) {
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
  Serial.println(" setup MPU6050 begin :");
  delay(2000);

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("");
    Serial.println(" Failed to find MPU6050 chip");
      delay(10);
  }
  Serial.println(" MPU6050 Found!");
  delay(2000);

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
  Serial.println("");
  Serial.println(" DONE : setup MPU6050");
  delay(2000);
}

/* ----------------------------------------------------------- loop ---------------------------------------------------------------*/
void loop() {
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  /* Print out the values */
  Serial.print(" Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.println(" [m/s^2]");

  Serial.print(" Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" [rad/s]");

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" [degC]");

  Serial.println("");
  delay(500);
}
