#ifndef __ASYNCSERIAL__ 
#define __ASYNCSERIAL__ 

#include <Arduino.h>

// These two routines are needed because the Serial.print method prints without using interrupts.
// Calls to Serial.print block interrupts and use a wait in kernel space causing all ISRs to
// be blocked and hence we could miss some timer interrupts.
// To avoid this problem call PushTxt to have stuff delivered to the serial line, PushTxt simply
// stores your text for future print out by PutChar. The PutChar routine removes one character
// from the stored text each time its called. By placing a call to PutChar in the outermost loop
// of the Arduino loop function, then for each loop one character is printed, avoiding blocking
// of interrupts and vastly improving the loops real time behaviour.

class AsyncSerial {
  // This is the text ring buffer for real time output to serial line with interrupt on
  static const int TBLEN = 1024;      // Serial line output ring buffer size, 8K
  char txtb[TBLEN];                // Text ring buffer
  uint32_t txtw, txtr,     // Write and Read indexes
                  tsze, terr,     // Buffer size and error code
                  tmax;               // The maximum size the buffer reached
  
  typedef enum { TXT_NOERR=0, TXT_TOOBIG=1, TXT_OVERFL=2 } TxtErr;
  
  public:
    AsyncSerial(int baudRate);
    // Copy text to the buffer for future printing
    void print(char *txt);
    // Take the next character from the ring buffer and print it, called from the main loop
    void PutChar();
  
  
};


#endif 
