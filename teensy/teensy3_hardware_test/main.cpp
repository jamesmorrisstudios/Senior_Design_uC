#include "WProgram.h"
#include "nativeSPI.h"
#include "mk20dx128.h"

byte buffer[2];
int state = HIGH;

extern "C" int main(void)
{
	//Stepper motor control pins
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	
	//Laser control pins
	pinMode(2, OUTPUT);
	
	//SPI control pins
	pinMode(9, OUTPUT);
	pinMode(10, OUTPUT);
	pinMode(11, OUTPUT);
	
	digitalWriteFast(0, state);
	digitalWriteFast(1, state);
	digitalWriteFast(2, state);
	digitalWriteFast(9, state);
	digitalWriteFast(10, state);
	
	spiInit(12);
	
	buffer[0] = 0x51;
	buffer[1] = 0x51;
	
	//Main loop
	while(1){
	delayMicroseconds(10);
		spiSend(buffer, 2);
		digitalWriteFast(0, state);
		digitalWriteFast(1, state);
		digitalWriteFast(2, state);
		digitalWriteFast(9, state);
		digitalWriteFast(10, state);
		state = !state;
		delay(1);
	}//main loop
}//main


