#ifndef __SENSORS__ 
#define __SENSORS__ 

#include <Arduino.h>
#include "asyncSerial.h"
// LPS lib from here: https://github.com/pololu/lps-arduino
#include "src/LPS.h"
// LSM303 lib from here: https://github.com/pololu/lsm303-arduino
#include "src/LSM303.h"
// HTU21D lib from here: https://github.com/sparkfun/SparkFun_HTU21D_Breakout_Arduino_Library
#include "src/SparkFunHTU21D.h"

class Sensors {
  // object for the pressure sensor
  LPS baro;
  // object for the accel and magnetometer
  LSM303 accelMagneto;
  // object for humidity sensor
  HTU21D humidity;
  
  // vars for checking if a sensor initilized correctly
  bool baroOK;
  bool accelMagnetoOK;
  bool humidOK;
  
  // cute class for printing and the needed char array
  AsyncSerial *aSer;
  static const int TXTLEN = 512;
  char txt[TXTLEN];                // For writing to serial
  
  public:
    Sensors(AsyncSerial *aS);
    // initilize sensors
    bool init();
    // different ways of printing data
    void printBaro();
    void printAccel();
    void printMagneto();
    void printHumid();
    void printTempAvg();
    void printAll();

  private:
    float AclToMs2(int16_t val);
    float MagToGauss(int16_t val);
  
};


#endif 
