#include <Arduino.h>

// class for interacting with the GPS
class GPS {

  // fields for data storage
  int year, month,  day;
  int hour, minute, second;
  int unixTime;
  float latitude = 0.0, longitude = 0.0, altitude = 0.0;
  
  // cute class for printing and the needed char array
  AsyncSerial *aSer;
  char txt[TXTLEN];                // For writing to serial
  
  public:
    GPS(AsyncSerial *aS);
    
};
  
  
