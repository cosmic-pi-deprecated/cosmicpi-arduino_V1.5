
// WARNING: One up the spout !!
// The GPS chip puts the next nmea string in its output buffer
// only if its been read, IE its empty.
// So if you read infrequently the string in the buffer is old and
// has the WRONG time !!! The string lies around like a bullet in
// the breach waiting for some mug.

boolean pipeGPS() {
  while (Serial1.available()) {
    char c[1];
    c[0] = Serial1.read();
    aSer->print(c); 
  }
}

// GPS setup

void GpsSetup() {

// definitions for different outputs
// only one of these strings can be used at a time
// otherwise they will overwrite each other
// for more information take a look at the QUECTEL L70 protocoll specification: http://docs-europe.electrocomponents.com/webdocs/147d/0900766b8147dbdd.pdf
#define RMCGGA    "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28" // RCM & GGA
#define ZDA    "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0*29" // ZDA
#define GGAZDA    "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0*28" // GGA & ZDA
#define GGA    "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" // GGA

// gets the firmware version 
#define FMWVERS   "$PMTK605*31"             // PMTK_Q_RELEASE
// Sets the update intervall
#define NORMAL    "$PMTK220,1000*1F"          // PMTK_SET_NMEA_UPDATE_1HZ
// disables updates for the antenna status (only Adafruit ultimate GPS?)
#define NOANTENNA "$PGCMD,33,0*6D"          // PGCMD_NOAN

  Serial1.println(NOANTENNA);
  Serial1.println(GGAZDA);
  Serial1.println(NORMAL);
  Serial1.println(FMWVERS);
}

