#include "asyncSerial.h"
#include <Arduino.h>


// Avoid being blocked in the serial write routine

AsyncSerial::AsyncSerial(int baudRate){
  Serial.begin(baudRate);
  txtw = 0;
  txtr = 0;
  tsze = 0;
  terr = 0;
  tmax = 0; 
}

// Copy text to the buffer for future printing
void AsyncSerial::print(char *txt) {

        int i, l = strlen(txt);

        // If this happens there is a programming bug
        if (l > TBLEN) {                // Can't handle more than TBLEN at a time
                terr = TXT_TOOBIG;      // say error and abort
                return;
        }

        // If the buffer is filling up to fast throw it away and return an error
        if ((l + tsze) >= TBLEN) {      // If there is no room in the buffer
                terr = TXT_OVERFL;      // Buffer overflow
                return;                 // Simply stop printing when txt comming too fast
        }

        // Copy the new text onto the ring buffer for later output
        // from the loop idle function
        for (i=0; i<l; i++) {
                txtb[txtw] = txt[i];            // Put char in the buffer and
                txtw = (txtw + 1) % TBLEN;      // get the next write pointer modulo TBLEN
        }
        tsze = (tsze + l) % TBLEN;              // new buffer size
        if (tsze > tmax) tmax = tsze;           // track the max size
}

// Take the next character from the ring buffer and print it, called from the main loop

void AsyncSerial::PutChar() {
        char c[2];                              // One character zero terminated string
        if ((tsze) && (!Serial.available())) {  // If the buffer is not empty and not reading
        //if ((tsze)) {  // If the buffer is not empty and not reading
                c[0] = txtb[txtr];              // Get the next character from the read pointer
                c[1] = '\0';                    // Build a zero terminated string
                txtr = (txtr + 1) % TBLEN;      // Get the next read pointer modulo TBLEN
                tsze = (tsze - 1) % TBLEN;      // Reduce the buffer size
                Serial.print(c);                // and print the character
        }
}
