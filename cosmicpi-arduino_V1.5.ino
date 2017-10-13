
#include "asyncSerial.h"
#include <Wire.h>

static const int SERIAL_BAUD_RATE = 115200;   // Serial baud rate
static const int GPS_BAUD_RATE = 9600;        // GPS and Serial1 line

// simulate events
static bool simulateEvents = true;
unsigned long  nextSimEvent = 0;

// start our async serial connection for global use
// it would normally work just fine as an instance
// but I want to pass around the pointer to different classes
// to make sure that only one instance is ever used
AsyncSerial *aSer;

// sensors for global use
#include "sensors.h"
Sensors sensors(aSer);

// LED pins
#define PPS_PIN 12      // PPS (Pulse Per Second) and LED
#define EVT_PIN 11      // Cosmic ray event detected

// Leds  flag
bool leds_on = true;

// time to print a sensor update (in ms before a PPL)
// the sensor update should always print inbetween the PPS, to avoid problems with the serial pipe from the GPS
static unsigned long distanceSensorUpdatePPS = 200;
unsigned long  nextSensorUpdate = 0;

// How long the event LED should light up (in ms)
static int event_LED_time = 15;


// string used for passing data to our asynchronous serial print class
static const int TXTLEN = 512;
static char txt[TXTLEN];                // For writing to serial  

static uint32_t displ = 0;      // Display values in loop

// Timer registers REGA....
static uint32_t rega1, stsr1 = 0;
static uint32_t stsr2 = 0;

boolean pll_flag = false;

long eventCount = 0;
unsigned long pps_micros = 0;
unsigned long target_mills = millis() + 1030;
unsigned long last_event_LED = 0;

#define FREQ 42000000                   // Clock frequency
#define MFRQ 40000000                   // Sanity check frequency value

// Timer chip interrupt handlers try to get time stamps to within 4 system clock ticks
static uint32_t rega0 = FREQ,   // RA reg
                stsr0 = 0,      // Interrupt status register
                ppcnt = 0,      // PPS count
                delcn = 0;      // Synthetic PPS ms

// GPS and time flags
boolean gps_ok = false;         // Chip OK flag
boolean pps_recieved = false;



// ---------------------- Timing

// Initialize the timer chips to measure time between the PPS pulses and the EVENT pulse
// The PPS enters pin D2, the PPS is forwarded accross an isolating diode to pin D5
// The event pulse is also connected to pin D5. So D5 sees the LOR of the PPS and the
// event, while D2 sees only the PPS. In this way we measure the frequency of the
// clock MCLK/2 each second on the first counter, and the time between EVENTs on the second
// I use a the unconnected timer block TC1 to make a PLL that is kept in phase by the PPS
// arrival in TC0 and which is loaded with the last measured PPS frequency. This PLL will
// take over the PPS generation if the real PPS goes missing.
// In this implementation the diode is implemented in software, see later


void TimersStart() {

        uint32_t config = 0;

        // Set up the power management controller for TC0 and TC2

        pmc_set_writeprotect(false);    // Enable write access to power management chip
        pmc_enable_periph_clk(ID_TC0);  // Turn on power for timer block 0 channel 0
        pmc_enable_periph_clk(ID_TC3);  // Turn on power for timer block 1 channel 0
        pmc_enable_periph_clk(ID_TC6);  // Turn on power for timer block 2 channel 0

        // Timer block 0 channel 0 is connected only to the PPS 
        // We set it up to load regester RA on each PPS and reset
        // So RA will contain the number of clock ticks between two PPS, this
        // value is the clock frequency and should be very stable +/- one tick

        config = TC_CMR_TCCLKS_TIMER_CLOCK1 |           // Select fast clock MCK/2 = 42 MHz
                 TC_CMR_ETRGEDG_RISING |                // External trigger rising edge on TIOA0
                 TC_CMR_ABETRG |                        // Use the TIOA external input line
                 TC_CMR_LDRA_RISING;                    // Latch counter value into RA

        TC_Configure(TC0, 0, config);                   // Configure channel 0 of TC0
        TC_Start(TC0, 0);                               // Start timer running

        TC0->TC_CHANNEL[0].TC_IER =  TC_IER_LDRAS;      // Enable the load AR channel 0 interrupt each PPS
        TC0->TC_CHANNEL[0].TC_IDR = ~TC_IER_LDRAS;      // and disable the rest of the interrupt sources
        NVIC_EnableIRQ(TC0_IRQn);                       // Enable interrupt handler for channel 0

        // Timer block 1 channel 0 is the PLL for when the GPS chip isn't providing the PPS
        // it has the frequency loaded in reg C and is triggered from the TC0 ISR

        config = TC_CMR_TCCLKS_TIMER_CLOCK1 |           // Select fast clock MCK/2 = 42 MHz
                 TC_CMR_CPCTRG;                         // Compare register C with count value

        TC_Configure(TC1, 0, config);                   // Configure channel 0 of TC1
        TC_SetRC(TC1, 0, FREQ);                         // One second approx initial PLL value
        TC_Start(TC1, 0);                               // Start timer running

        // Timer block 2 channel 0 is connected to the RAY event
        // It is kept in phase by the PPS comming from TC0 when the PPS arrives
        // or from TC1 when the PLL is active (This is the so called software diode logic)

        config = TC_CMR_TCCLKS_TIMER_CLOCK1 |           // Select fast clock MCK/2 = 42 MHz
                 TC_CMR_ETRGEDG_RISING |                // External trigger rising edge on TIOA1
                 TC_CMR_ABETRG |                        // Use the TIOA external input line
                 TC_CMR_LDRA_RISING;                    // Latch counter value into RA

        TC_Configure(TC2, 0, config);                   // Configure channel 0 of TC2
        TC_Start(TC2, 0);                               // Start timer running

        TC2->TC_CHANNEL[0].TC_IER =  TC_IER_LDRAS;      // Enable the load AR channel 0 interrupt each PPS
        TC2->TC_CHANNEL[0].TC_IDR = ~TC_IER_LDRAS;      // and disable the rest of the interrupt sources
        NVIC_EnableIRQ(TC6_IRQn);                       // Enable interrupt handler for channel 0

        // Set up the PIO controller to route input pins for TC0 and TC2

        PIO_Configure(PIOC,PIO_INPUT,
                      PIO_PB25B_TIOA0,  // D2 Input     
                      PIO_DEFAULT);

        PIO_Configure(PIOC,PIO_INPUT,
                      PIO_PC25B_TIOA6,  // D5 Input
                      PIO_DEFAULT);
}

// Dead time is the time to wait after seeing an event
// before detecting a new event. There is ringing on the
// event input on pin 5 that needs suppressing

uint32_t old_ra = 0;    // Old register value from previous event
uint32_t new_ra = 0;    // New counter value that must be bigger by dead time
uint32_t dead_time = 420000;  // 10ms
uint32_t dead_cntr = 0;   // Suppressed interrupts due to dead time
uint32_t dead_dely = 0; // Amout of time lost in dead time

// Handle the PPS interrupt in counter block 0 ISR

void TC0_Handler() {

        // reset pps_millis
        pps_micros = micros();
        // disable our backup "timer"
        target_mills = millis() + 1010;

        // In principal we could connect a diode
        // to pass on the PPS to counter blocks 1 & 2. However for some unknown
        // reason this pulls down the PPS voltage level to less than 1V and
        // the trigger becomes unreliable !! 
        // In any case the PPS is 100ms wide !! Introducing a blind spot when
        // the diode creates the OR of the event trigger and the PPS.
        // So this is a software diode

        TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG; // Forward PPS to counter block 2
        TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG; // Forward PPS to counter block 1

        rega0 = TC0->TC_CHANNEL[0].TC_RA;       // Read the RA reg (PPS period)
        stsr0 = TC_GetStatus(TC0, 0);           // Read status and clear load bits

        if (rega0 < MFRQ)                       // Sanity check against noise
                rega0 = FREQ;                   // Use nominal value

        TC_SetRC(TC1, 0, rega0);                // Set the PLL count to what we just counted

        ppcnt++;                                // PPS count
        gps_ok = true;                          // Its OK because we got a PPS  
        pll_flag = true;                        // Inhibit PLL, dont take over PPS arrived
        pps_recieved = true;

        old_ra = 0;                             // Dead time counters
        new_ra = 0;
        dead_dely = 0;                          // Reset dead delay

        if (leds_on) {
          digitalWrite(PPS_PIN, !digitalRead(PPS_PIN));   // Toggle led
        }
}

// Handle PLL interrupts
// When/If the PPS goes missing due to a lost lock we carry on with the last measured
// value for the second from TC0

void TC3_Handler() {

        stsr2 = TC_GetStatus(TC1, 0);           // Read status and clear interrupt

        if (pll_flag == false) {                // Only take over when no PPS

                TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG; // Forward PPS to counter block 2
                ppcnt++;                                // PPS count
                displ = 1;                              // Display stuff in the loop
                gps_ok = false;                         // PPS missing
        }
        pll_flag = false;                               // Take over until PPS comes back
}

// Handle isolated PPS (via diode) LOR with the Event
// The diode is needed to block Event pulses getting back to TC0
// LOR means Logical inclusive OR
// Now we are using the software diode implementation

void TC6_Handler() {

  unsigned long us = micros() - pps_micros;
  eventCount++;
  sprintf(txt,"Event: sub second micros:%d; Event Count:%d\n", us, eventCount);
  aSer->print(txt);

  // turn on LED, it will be turned off in the main loop
  last_event_LED = millis();
  if (leds_on) {
    digitalWrite(EVT_PIN, HIGH);
  }

  
  // Then unblock
        rega1 = TC2->TC_CHANNEL[0].TC_RA;       // Read the RA on channel 1 (PPS period)
        stsr1 = TC_GetStatus(TC2, 0);           // Read status clear load bits

}


// Push the GPS state when we recieve a PPS or PPL
void printPPS() {
  sprintf(txt,"PPS: GPS lock:%d;\n", gps_ok);
  aSer->print(txt);
}

// Things that need handling on PPS and PLL, but are non time critical
void PPL_PPS_combinedHandling(){
  printPPS();
  // set the time for the next sensor update
  nextSensorUpdate = target_mills - distanceSensorUpdatePPS;
}


// ------------------------- Arudino Functions

// Arduino setup function, initialize hardware and software
// This is the first function to be called when the sketch is started

void setup() {

  if (simulateEvents) {
    randomSeed(42);
  }

  // The two leds on the front pannel for PPS and Event
  if (leds_on) {
    pinMode(EVT_PIN, OUTPUT);       // Pin for the cosmic ray event 
    pinMode(PPS_PIN, OUTPUT);       // Pin for the PPS (LED pin)
  }

  TimersStart();  // Start timers
  target_mills = millis() + 1010; // backup PPS

  aSer = new AsyncSerial(SERIAL_BAUD_RATE); // Start the serial line
  // start the GPS
  Serial1.begin(GPS_BAUD_RATE);
  GpsSetup();

  // start the i2c bus
  Wire.begin();

  // start the detector
  setupDetector();

  // start the sensors
  sensors = Sensors(aSer);

  if (leds_on) {
    digitalWrite(PPS_PIN, HIGH);   // Turn on led
  }

  // initilize the sensors
  if(!sensors.init()){
    aSer->print("WARNING: Error in sensor initialization - continuing - output may be inclomplete!\n");
  } else{
    aSer->print("INFO: Sensor setup complete\n");
  }

  aSer->print("INFO: Running\n");
}


// Arduino main loop does all the user space work

void loop() {

  // on a PPS from the GPS
  if (pps_recieved){
    PPL_PPS_combinedHandling();
    pps_recieved = false;
  }

  // if no pps recieved
  if (millis() >= target_mills){
    target_mills = millis() + 1000;
    // reset pps_millis
    pps_micros = micros();
    // while we have no pps we will keep the LED solid
    if (leds_on) {
      digitalWrite(PPS_PIN, HIGH);
    }
    gps_ok = false; 
    PPL_PPS_combinedHandling();
  }

  // simulate an interrupt if we want to simulate events
  if (simulateEvents) {
    if (millis() >= nextSimEvent){
      TC6_Handler();
      nextSimEvent = millis() + random(100, 2000);
    }
  }
  
  // reset event LED when enough time has passed
  if (millis() >= (last_event_LED + event_LED_time)){
    if (leds_on) {
      digitalWrite(EVT_PIN, LOW);
    }
  }

  
  // print out sensor updates
  if (millis() >= (nextSensorUpdate)){
    sensors.printAll();
    nextSensorUpdate += 1000;
  }

  // pipe GPS if it's available
  pipeGPS();

  aSer->PutChar();      // Print one character per loop !!!
}



