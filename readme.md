# CosmicPi V1.5 Arduino DUE software

This is the Arduino DUE software running on the CosmicPi V1.5.
This software is designed to work in a plug and play fashion, no interaction is required.


## Features
*	Set up the detector with default values
*	Send events from the detector via serial
*	Send data from all sensors on the board via serial
*	Configure the on board GPS and pipe it's data to serial

### Meaning of the LEDs
*	Lower LED (green): Power and GPS
	*	Solid: Power, but no GPS lock
	*	Blinking: Power and GPS lock
*	Upper LED (red): Event
	*	Flash: An Event has been registered

	
## ToDo before release
*	The timer needs a rework, its current implementation is imprecise
*	The detector should be able to calibrate it's parameters automatically
	*	High voltage
	*	Thresholds

	
## Installation
For regular users of the CosmicPi V1.5 this should be taken care of automatically by the software on the RaspberryPi.

For everybody interested in looking into developing this oneself:
1.	Download the most recent Arduino IDE: https://www.arduino.cc/en/main/software
2. 	Install the SAM core: https://www.arduino.cc/en/Guide/Cores
3. 	Clone this repository: `git clone https://github.com/CosmicPi/cosmicpi-arduino_V1.5.git`
(note you can also download the .zip file from the menu above to the right, expand and remember to re-name the directory so cosmpicpi-arduino_v1.5 otherwise Arduino will give you errors)
4.	Open the file `cosmicpi-arduino_V1.5.ino` with the Arduino IDE
5.	Connect your CosmicPi to your computer via the USB **Programming** port
6. 	Select the newly appearing port in the Arduino IDE (Tools -> Port)
7.	Compile and upload the firmware (Sketch -> Upload)


## Usage
As soon as the CosmicPi is connected you can open the software of your choice for monitoring serial data. Open the Arduinos serial port with a baudrate of 19200.
This firmware is designed to be a plug and play firmware. 
It will only send data. It will not accept inputs. What data is sent is defined [here](https://github.com/CosmicPi/cosmicpi-rpi_V1.5/blob/master/documentation/CosmicPi_V15_serial_comm.txt).

