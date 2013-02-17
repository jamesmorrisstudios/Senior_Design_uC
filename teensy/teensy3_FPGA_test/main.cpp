#include "WProgram.h"
#define MAXHEIGHT 480

#define STATEIDLE 0
#define STATESCAN 1

#define nop asm volatile ("nop\n\t")

volatile byte byteBuffer1 = 0x03;
volatile byte byteBuffer2 = 0xE8;

volatile int state;
volatile int bytesSent;
volatile int bitCount = -1;

void startScan(){
	bytesSent = 0;
	bitCount = -1;
	state = STATESCAN;
}

void sendValue(){
	if(bitCount <8){
		digitalWriteFast(12, byteBuffer2 & (1 << bitCount));
		
	}else{
		digitalWriteFast(12, byteBuffer1 & (1 << bitCount-8));
	
	}
	bitCount--;
}


extern "C" int main(void)
{
	state = STATEIDLE;
	
	pinMode(10, INPUT); //primary uC pulls down to signal start of a scan
	digitalWrite(10, HIGH);
	attachInterrupt(10, startScan, FALLING);
	
	pinMode(9, OUTPUT); //FPGA (this) pulls down when ready to transmit a byte
	digitalWrite(9, HIGH);
	
	pinMode(13, INPUT); //sclock
	attachInterrupt(13, sendValue, RISING);
	
	pinMode(12, OUTPUT);//MISO
	digitalWriteFast(12, LOW);
	
	
	pinMode(11, INPUT); //MOSI Unused for now
	
	//byteBuffer = 0x1D;
	
	//Main loop
	while(1){
		//IDLE
		//Wait for command to start scanning
		if (state == STATEIDLE){
			//do nothing ISR handles starting
		}else if (state == STATESCAN){
			while(bytesSent < MAXHEIGHT * 2){
				if(bitCount < 0){
				//delaymicroseconds(1);
					nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
					nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
					nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
					nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
					nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
					nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
					nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
					nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
					nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
					nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
					digitalWriteFast(12, LOW);
					bitCount = 15;
					bytesSent++;
					
					digitalWriteFast(9, LOW);
					sendValue();
					nop;
					nop;
					digitalWriteFast(9, HIGH);	
				}
				//send a depth of 1234 to the primary uC for all 480 vertical lines
				
				//for(int i=0;i<MAXHEIGHT;i++){
				//	readRegister(REG_WHOAMI, 2, byteBuffer, byteOutBuffer);
				//}
				//Send the END command to the primary uC
				//byteOutBuffer[0] = 0x80;
				//byteOutBuffer[1] = 0x00;
				//readRegister(REG_WHOAMI, 2, byteBuffer, byteOutBuffer);
				//go back to idle
			}
			state = STATEIDLE;
		}
	}//main loop
}//main


