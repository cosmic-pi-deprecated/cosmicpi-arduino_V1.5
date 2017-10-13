#include <Wire.h>

// configure these to configure the detector
static const int HV_DEFAULT = 104;
static const int DEFAULT_DAC_THRESH = 1600;

// other constants
static const int HV_MAX = 89;
static const int HV_MIN = 255;
static const int DEFAULT_THRESH = 100;
static const bool USE_DAC = true;

//set up the pins to remap SPI by hand
static const int num_devices = 2;
static const int SS_pin[num_devices] = {14, 15};
static const int SCK_pin = 17;
//static const int MISO_pin = 22;
static const int MOSI_pin = 16;

// I2C adress pins
#define MAX5387_PA0_pin A9
#define MAX5387_PA1_pin A10
#define MAX5387_PA2_pin A11

byte thresh1;
byte thresh2;

// initilizes the detector with default values
void setupDetector(){
  // setup pins
  detecSetPinModes();
  detcSetConstantPins();
  // set defaults
  if (USE_DAC){
    analogWrite(DAC0, DEFAULT_DAC_THRESH);
    analogWrite(DAC1, DEFAULT_DAC_THRESH); 
  }else{
    setThreshold(3, DEFAULT_THRESH);
  }
  setHV(HV_DEFAULT);
  sprintf(txt,"Detector setup finished\n");
  aSer->print(txt);
}


// sets pin modes needed for the detector
void detecSetPinModes(){
  //setup analog writemode
  analogWriteResolution(12);
  // I2C adress pins for the MAX5387
  pinMode(MAX5387_PA0_pin, OUTPUT);
  pinMode(MAX5387_PA1_pin, OUTPUT);
  pinMode(MAX5387_PA2_pin, OUTPUT);
  // HV pins
  digitalWrite(SS, HIGH);  // Start with SS high
  for (int i=0; i<num_devices; i++){
    pinMode(SS_pin[i], OUTPUT);
    digitalWrite(SS_pin[i], HIGH);  
  }
  pinMode(SCK_pin, OUTPUT);
  //pinMode(MISO_pin, INPUT); //this is the avalanche pin, not implemented yet
  pinMode(MOSI_pin, OUTPUT);
}


// sets pins that don't need changing (ever)
void detcSetConstantPins(){
  // I2C adress pins for the MAX5387
  digitalWrite(MAX5387_PA0_pin, LOW);//configure the address of the MAX5387 pot
  digitalWrite(MAX5387_PA1_pin, LOW);//configure the address of the MAX5387 pot
  digitalWrite(MAX5387_PA2_pin, LOW);//configure the address of the MAX5387 pot
}


// this function sets the thresholds for the MAX5387
// 1 is the first channel, 2 the second and 3 sets both at the same time
void setThreshold(int pot_channel, int value){
  // do a value check
  if (value > 255 || value < 1){
    return;
  } else {
    value = byte(value);
  }
  
  Wire.begin();
  Wire.beginTransmission(byte(0x28)); // transmit to device #112
  switch(pot_channel){
    case 1:
      sprintf(txt,"Setting threshold on channel 1 to: %d\n", value);
      aSer->print(txt);
      Wire.write(byte(B00010001)); //sets value to the first channel
      Wire.write(value);
      thresh1 = value;
      break;
    case 2:
      sprintf(txt,"Setting threshold on channel 2 to: %d\n", value);
      aSer->print(txt);
      Wire.write(byte(B00010010)); //sets value to the second channel
      Wire.write(value);
      thresh2 = value;
      break;
    case 3:
      sprintf(txt,"Setting threshold on channel 1&2 to: %d\n", value);
      aSer->print(txt);
      Wire.write(byte(B00010011)); //sets value to both channels
      Wire.write(value);
      thresh1 = value;
      thresh2 = value;
      break;
  }
  
  Wire.endTransmission();
}


// set the two HV supplies
byte setHV(byte _send)  // This function is what bitbangs the data
{
  sprintf(txt,"INFO: Setting HV 1&2 to: %d\n", _send);
  aSer->print(txt);
  //reception isn't implemented in this version. 
  //byte _receive = 0;
  for(int j=0; j<num_devices; j++){
    digitalWrite(SS_pin[j], LOW);        // SS low 
    for(int i=0; i<8; i++)  // There are 8 bits in a byte
    {
      digitalWrite(MOSI_pin, bitRead(_send, 7-i));    // Set MOSI
      //delay(1);
      digitalWrite(SCK_pin, HIGH);                  // SCK high
      //bitWrite(_receive, i, digitalRead(MISO_pin)); // Capture MISO
      digitalWrite(SCK_pin, LOW);                   // SCK low
    //digitalWrite(MOSI_pin, LOW);    // Set MOSI
      
    }
     digitalWrite(SS_pin[j], HIGH);       // SS high again 
  }
  //return _receive;        // Return the received data
}
