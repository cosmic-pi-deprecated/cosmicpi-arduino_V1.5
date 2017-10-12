
// This is the nmea data string from the GPS chip
#define GPS_STRING_LEN 256
static char gps_string[GPS_STRING_LEN + 1];

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

#define RMCGGA    "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28" // RCM & GGA
#define RMCZDA    "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0*29" // ZDA
#define GGA_AND_ZDA    "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0*28" // GGA & ZDA
#define GGA    "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" // GGA
#define FMWVERS   "$PMTK605*31"             // PMTK_Q_RELEASE
#define NORMAL    "$PMTK220,1000*1F"          // PMTK_SET_NMEA_UPDATE_1HZ
#define NOANTENNA "$PGCMD,33,0*6D"          // PGCMD_NOAN

  Serial1.println(NOANTENNA);
  Serial1.println(GGA_AND_ZDA);
  Serial1.println(NORMAL);
  Serial1.println(FMWVERS);
}

