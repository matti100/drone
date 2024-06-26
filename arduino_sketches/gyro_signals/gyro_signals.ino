////////////////////////////////////////
/////// GYRO ANGULAR RATES /////////////
////////////////////////////////////////
/*
*   This script is meant to be a quick reference for the implementation of
*   a more sofisticated gyroscope measurement algorithm for the acqisition
*   of angular rates (Roll/Pitch/Yaw rates).
* 
*   DEVICES:
*   - Arduino Uno R3
*   - MPU6050 (IMU sensor)
* 
*   LIBRARIES:
*   - Wire.h
*
*   BRIEF EXPLANATION of the code:
*   1) we need to access the internal registers of the IMU sensor in order to:
*   - set important parameters and settings
*   - reading data 
*
*   This type of devices are divided into "internal registers" that are responsible for different actions, such
*   as storing data, settings ecc.
*   Each internal register has an unique "address" (generally written in hexadecimal format)
*   and each register contains a total of 8 bits (from bit0 to bit7) that can store informations
*   It is possible to write and read data from the register in an hexadecimal format (instead of sending data in binary,
*   which can be very impractical).
*   
*   Informations might be stored in more than one (consecutive) register, otherwise the data read by Arduino are meaningless.
*   Regarding gyro measurements, angular rates are often stored in 16 bits (2 registers).
*   It is suggested to reference to the device "Register Map" in order to identify the correct addresses of the registers,
*   other than the position of the most significan bit.
*
*  2) 



/* ----------------------- Libraries ----------------------- */

// Library for I2C communication 
#include <Wire.h>

/* ---------------------- Global Variables ------------------- */

#define MPU6050_ADDRESS 0x68                // Address associated with MPU6050
#define LOW_PASS_FILTER_ADDRESS 0x1A        // Address to access Low Pass Filter Register in MPU6050
#define SENSITIVY_ADDRESS 0x1B              // Address to access Sensitivity settings in MPU6050
#define GYRO_REGISTER_ADDRESS 0x43          // Address to access Gyro Measurements in MPU6050

float RollRate;                             // Roll, Pitch, Yaw rates
float PitchRate;
float YawRate;

float RateCalibrationRoll = 0;              // Temporary variables used to calibrate the angular rates
float RateCalibrationPitch = 0;             //
float RateCalibrationYaw = 0;               // The aim is to delete the zero mean error of the measurements.
int RateCalibrationNumber;

/* ------------------------- Functions --------------------------- */

// SETUP AND RECEIVE SIGNALS FROM GYROSCOPE
void gyro_signals() {                        
  
  // Switch on the LOW PASS FILTER 
  Wire.beginTransmission(MPU6050_ADDRESS);    // starting communication with MPU6050
  Wire.write(LOW_PASS_FILTER_ADDRESS);        // accessing the internal register responsible for the Low Pass Filter
  Wire.write(0x05);                           // sending data in hexadecimal format to turn on the LPF
  Wire.endTransmission();

  // Set the sensitivity scale factor
  Wire.beginTransmission(MPU6050_ADDRESS);    // starting communication with MPU6050
  Wire.write(SENSITIVY_ADDRESS);              // accessing the internal register for Sensitivity
  Wire.write(0x08);                           // sending data in hexadecimal format to set the desidered sensitivity
  Wire.endTransmission();
  
  // GYRO MEASUREMENTS
  Wire.beginTransmission(MPU6050_ADDRESS);    // starting communication with MPU6050
  Wire.write(GYRO_REGISTER_ADDRESS);          // accessing the internal register for gyroscope measurements
  Wire.endTransmission();
  // now we are set to the gyro internal register

  // Request Data
  Wire.requestFrom(MPU6050_ADDRESS, 6);             // requesting 6 bytes from MPU6050
  
  int16_t GyroX = Wire.read() << 8 | Wire.read();   // the data of the X rotation are formatted in 16 bits (2 bytes).
                                                    // The << 8 operator move the 8 bits received from the first
                                                    // Wire.read() to the left of 8 positions.
                                                    // The | operator lets the following 8 bits to join the previous 8 bits received
                                                    // Wire.read() let us receive the last 8 bits.
                                                    // GyroX now is a variable composed of 16 bits (2 bytes)
  int16_t GyroY = Wire.read() << 8 | Wire.read();
  int16_t GyroZ = Wire.read() << 8 | Wire.read();
  // All the data about the X, Y and Z rotation has been received.

  // Convert into [° / s]
  RollRate = (float)GyroX / 65.5;
  PitchRate = (float)GyroY / 65.5;
  YawRate = (float)GyroZ / 65.5;
}

// SETUP 
void setup() {
  // Start Serial Communication with computer
  Serial.begin(57600);

  // Set parameters for the I2C communication with MPU6050
  Wire.setClock(400000);                    // set the clock frequency (how many times in a second Arduino communicate
                                            // with the sensor)
  Wire.begin();                           
  delay(250);

  Wire.beginTransmission(MPU6050_ADDRESS);  // starting communication with MPU6050
  Wire.write(0x6B);                         // accessing the internal register responsible for Power Management Mode
  Wire.write(0x00);                         // sending data in hexadecimal format to set Sleep Mode off
  Wire.endTransmission();

  // Starting Calibration Procedure
  Serial.print("\n\nCalibrating. Don't move the sensor");

  // Measuring data for calibration
  for (RateCalibrationNumber = 0; RateCalibrationNumber < 5000; RateCalibrationNumber ++) {
    gyro_signals();                         // measuring

    RateCalibrationRoll += RollRate;        // adding 5000 angular rates measurements
    RateCalibrationPitch += PitchRate;
    RateCalibrationYaw += YawRate;
    delay(1);
  }

  RateCalibrationRoll /= 5000;              // computing the avarage of the first 5000 measurements 
  RateCalibrationPitch /= 5000;             // They represent the zero mean noise 
  RateCalibrationYaw /= 5000;               // associated to the sensor

}

// LOOP
void loop() {
  gyro_signals();                           // measuring angular rates

  RollRate -= RateCalibrationRoll;          // update Roll rate with calibration data
  PitchRate -= RateCalibrationPitch;        // update Pitch angle with calibration data
  YawRate -= RateCalibrationYaw;            // update Yaw angle with calibration data

  Serial.print("\n\n-------------------------\n");
  Serial.print("Roll Rate [° /s]: ");
  Serial.print(RollRate);
  Serial.print("\t\tPitch Rate [° / s]: ");
  Serial.print(PitchRate);
  Serial.print("\tYaw Rate [° / s]: ");
  Serial.print(YawRate);

  delay(50);

}
