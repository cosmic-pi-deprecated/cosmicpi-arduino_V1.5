#include "sensors.h"
#include <Arduino.h>
#include "asyncSerial.h"
// LPS lib from here: https://github.com/pololu/lps-arduino
#include "src/LPS.h"
// LSM303 lib from here: https://github.com/pololu/lsm303-arduino
#include "src/LSM303.h"
// HTU21D lib from here: https://github.com/adafruit/Adafruit_HTU21DF_Library
#include "src/SparkFunHTU21D.h"

Sensors::Sensors(AsyncSerial *aS){
  aSer = aS;
}

// initilize sensors
bool Sensors::init(){
  bool return_val = true;
  baroOK = baro.init();
  if (!baroOK)
  {
    sprintf(txt,"WARINING: Couldn't initilize the barometer!\n");
    aSer->print(txt);
    return_val = false;
  } else{
    baro.enableDefault();
  }
  
  accelMagnetoOK = accelMagneto.init();
  if (!accelMagnetoOK)
  {
    sprintf(txt,"WARINING: Couldn't initilize the accelerometer and magnetometer!\n");
    aSer->print(txt);
    return_val = false;
  } else{
    accelMagneto.enableDefault();
  }

  humidOK = humidity.begin();
  if (!humidOK)
  {
    sprintf(txt,"WARINING: Couldn't initilize the humidity sensor!\n");
    aSer->print(txt);
    return_val = false;
  }

  return return_val;  
}


// different ways of printing data
void Sensors::printBaro(){
  if (baroOK) {
    float pressure = baro.readPressureMillibars();
    float altitude = baro.pressureToAltitudeMeters(pressure);
    float temperature = baro.readTemperatureC();
        
    sprintf(txt,"Pressure: %f;\nAltitude: %f;\nTemperatureCBaro: %f;\n", pressure, altitude, temperature);
    aSer->print(txt);  
  }  
}

void Sensors::printAccel(){
  if (accelMagnetoOK) {
    accelMagneto.read();
    float x = AclToMs2(accelMagneto.a.x);
    float y = AclToMs2(accelMagneto.a.y);
    float z = AclToMs2(accelMagneto.a.z);
    sprintf(txt,"AccelX: %f;\nAccelY: %f;\nAccelZ: %f;\n", x, y, z);
    aSer->print(txt);  
  }  
}

void Sensors::printMagneto(){
  if (accelMagnetoOK) {
    accelMagneto.read();
    float x = MagToGauss(accelMagneto.m.x);
    float y = MagToGauss(accelMagneto.m.y);
    float z = MagToGauss(accelMagneto.m.z);
    sprintf(txt,"MagX: %f;\nMagY: %f;\nMagZ: %f;\n", x, y, z);
    aSer->print(txt);  
  }  
}

void Sensors::printHumid(){
  if (humidOK) {
    sprintf(txt,"TemperatureCHumid: %f;\nHumidity: %f;\n", humidity.readTemperature(), humidity.readHumidity());
    aSer->print(txt);  
  }  
}

void Sensors::printTempAvg(){
  short count = 0;
  int sum = 0;
  if (humidOK) {
    count++;
    sum += humidity.readTemperature();
  }
  if (baroOK) {
    count++;
    sum += baro.readTemperatureC();
  }  

  if (count != 0){
    float out = sum / count;
    sprintf(txt,"TemperatureC: %f;\n", out);
    aSer->print(txt);  
  }  
}

void Sensors::printAll(){
  printBaro();
  printAccel();
  printMagneto();
  printHumid();
  printTempAvg();
}

static const double GEARTH = 9.80665;
// normaly there is no - in ACL_FS, but the Accel is on upwards down, so it was necessary to get correct values
static const double ACL_FS = -2.0;  // +-2g 16 bit 2's compliment

// Convert to meters per sec per sec

float Sensors::AclToMs2(int16_t val) {
  return (ACL_FS * GEARTH) * ((float) val / (float) 0x7FFF);
}

#define MAGNETIC_FS 4.0  // Full scale Gauss 16 bit 2's compliment

float Sensors::MagToGauss(int16_t val) {
  return (MAGNETIC_FS * (float) val) / (float) 0x7FFF;
}

