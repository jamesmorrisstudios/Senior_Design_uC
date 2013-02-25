#include "WProgram.h"
#define USB_RAWHID
#include "usb_rawhid.h"
#include "nativeSPI.h"
#include "mk20dx128.h"

#define BUFSIZE 64
#define PACKETSIZE 2

byte cmd;

char data_in[BUFSIZE] = {0};
char data_out[BUFSIZE] = {0};

volatile byte byteBuffer[980];
volatile int inByte = 0;
volatile int outByte = 0;
byte buffer[2];

//Function declarations
void setupSPI();
int packData();


void setupSPI(){
	spiInit(12);
}

int packData(){
	
	//Loop over over the size of the data packet and break if the end of the source data is reached
	for(int i=0;i<BUFSIZE/PACKETSIZE;i++){
		data_out[0+i*PACKETSIZE] = byteBuffer[0+outByte];
		data_out[1+i*PACKETSIZE] = byteBuffer[1+outByte];
		outByte += 2;
	}
	inByte = 0;
	return 1;
}

//ISR for recieving data from the FPGA
void recieveValue(){
	spiRec(buffer, 2);
	byteBuffer[inByte] = buffer[0];
	byteBuffer[inByte+1] = buffer[1];
	inByte += 2;
}

extern "C" int main(void)
{
	setupSPI();
	
	pinMode(9, INPUT);
	//attachInterrupt(9, recieveValue, FALLING);
	
	byteBuffer[4] = 5;
	byteBuffer[2] = 10;;
	
	//Main loop
	while(1){
		inByte = 64;
		if(inByte >= 64){
			outByte = 0;
			packData();
			usb_rawhid_send(data_out, 15);
		}
	}//main loop
}//main


