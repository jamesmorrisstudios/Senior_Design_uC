
#include "WProgram.h"
#define USB_RAWHID
#include "usb_rawhid.h"
#include <SPI.h>

uint8_t test = 5;

int chipSelectPin = 10;
int REG_WHOAMI = 0x0F;
int REG_CTRL_REG1 = 0x20;
int REG_CTRL_REG4 = 0x23;
int REG_OUT_X_L = 0x28;

byte byteBuffer[6];

//
//SPI transfer function
//
void readRegister(byte registerBits, int numBytes, byte *byteBuffer) {
	  //digitalWrite(chipSelectPin, LOW);  //Take the chip select low to select the device:
	  for(int i = 0; i < numBytes; i++) {
		byteBuffer[i] = SPI.transfer(0xFF);  //Send dummy value to read output
	  }
	  //digitalWrite(chipSelectPin, HIGH);
}


extern "C" int main(void)
{

	//state == 0: wait for scan start signal
	//state == 1; scanning... transmitting data
	//state == 2: scan complete cleanup and reset to state 0
	int state = 0;

	//All serial data in and out is packed into 4 bytes per packet (32 bits)
	//the first 2 bits are command bits and are checked first
	char data_in[] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	byte data_out[] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

	byte cmd;
	int counter = 0;
	int type = 0;

	int rotation = 0;
	int height = 0;
	int depth = 1234;
	
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV2); //SPI_CLOCK_DIV32
	//SPI.begin();  //Start the SPI library:
	pinMode(chipSelectPin, OUTPUT);  //Initalize the data ready and chip select pins:
	
	pinMode(13, OUTPUT);
	
	while(1){
	
	  //Wait for available data
	  if (state == 0){
	  
		//if 4 or more bytes are available
		//if(Serial.available() >=4){
		if(usb_rawhid_available() >= 64){
			usb_rawhid_recv(data_in, 15);
		
		  //Serial.readBytes(data_in, sizeof(data_in)); //BUG? only works on char array not byte array
		  cmd = data_in[0] >> 6;
		  if(cmd == 1){
			 data_out[0] = 64;
			 //Serial.write(data_out, sizeof(data_out));
			 usb_rawhid_send(data_out, 15);
			 state = 1;
		  }
		}
		
		
	  //scanning... transmitting data
	  }else if (state == 1){
	  
	  //Transfer 4 dummy bytes via SPI just to check speed
	//readRegister(REG_WHOAMI, 4, byteBuffer);
	  
	  
		//if(Serial.available() >=4){
		if(usb_rawhid_available() >=4){
			usb_rawhid_recv(data_in, 15);
		
		  //Serial.readBytes(data_in, sizeof(data_in)); //BUG? only works on char array not byte array
		  cmd = data_in[0] >> 6;
		  if(cmd == 1){
			 data_out[0] = 64;
			 //Serial.write(data_out, sizeof(data_out));
			 usb_rawhid_send(data_out, 15);
			 state = 1;
		  }
		}
		
		data_out[0] = rotation >> 3;
		data_out[1] = (height >> 4) + ((rotation & 7) << 5);
		data_out[2] = (depth >> 8) + ((height & 15) << 4);
		data_out[3] = depth & 255;
		
		//Serial.write(data_out, sizeof(data_out));
		usb_rawhid_send(data_out, 15);
		height++;
		if(height >= 480){
		  rotation++;
		  height = 0;
		}
		if(rotation == 180){
		 state = 2; 
		}
		
	  //scan complete cleanup and reset to state 0
	  }else if (state == 2){
	  
		height = 0;
		rotation = 0;
		data_out[0] = 128;
		data_out[1] = 0;
		data_out[2] = 0;
		data_out[3] = 0;
		usb_rawhid_send(data_out, 15);
		//Serial.write(data_out, sizeof(data_out));
		state = 0;
		
	  }
	  //digitalWriteFast(13, HIGH);
	  //delay(100);
	  //digitalWriteFast(13, LOW);
	  //delay(100);
	}
	
}


