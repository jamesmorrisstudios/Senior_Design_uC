#include "WProgram.h"
#define USB_RAWHID
#include "usb_rawhid.h"
#include "mk20dx128.h"

#define BUFSIZE 64
#define PACKETSIZE 2

#define MAXHEIGHT 480
#define MAXROTATION 180

#define CMDSTART 64
#define CMDEND 128

#define STATEIDLE 0
#define STATEINITIATE 1
#define STATESCAN 2
#define STATEEND 3

byte cmd;

int rotation = 0;
int height = 0;
int depth = 1000;

char data_in[BUFSIZE] = {0};
char data_out[BUFSIZE] = {0};

int checkIncomingCmd(){
	if(usb_rawhid_available() >= BUFSIZE){
		return 1;
	}else{
		return -1;
	}
}

//
//Return 1 to start scan, 2 to end scan
//
int checkPCCmd(){
	usb_rawhid_recv(data_in, 15);
	cmd = data_in[0] >> 6;
	if(cmd == 1){
		return 1;
	}else if (cmd == 2){
		return 2;
	}
	return -1;
}

void sendPCStart(){
	data_out[0] = CMDSTART;
	usb_rawhid_send(data_out, 15);
}

void sendPCEnd(){
	data_out[0] = CMDEND;
	usb_rawhid_send(data_out, 15);
}

void resetState(){
	height = 0;
	rotation = 0;
	depth = 1000;
}

int packData(){
	int i;
	int ending = -1;

	//Loop over over the size of the data packet and break if the end of the source data is reached
	for(i=0;i<BUFSIZE/PACKETSIZE;i++){
		data_out[0+i*PACKETSIZE] = depth >> 8;
		data_out[1+i*PACKETSIZE] = depth & 255;

		if(ending == 1){return 1;}
		
		height++;
		if(height == MAXHEIGHT){
			rotation++;
			height = 0;
		}
		if(rotation == MAXROTATION){
			ending = 1;
		}
	}
	return 1;
}

extern "C" int main(void)
{
	//state == 0: IDLE wait for scan start signal
	//state == 1: INITIATE SCAN: reset all data and start new scan
	//state == 2; scanning... transmitting data
	//state == 3: scan complete cleanup and reset to state 0
	int state = STATEIDLE;

	//Main loop
	while(1){
		//IDLE
		//Wait for command to start scanning
		if (state == STATEIDLE){
			if(checkIncomingCmd() == 1){
				if(checkPCCmd() == 1){
					sendPCStart();
					state = STATEINITIATE;
				}
			}
		//INITIATE
		}else if(state == STATEINITIATE){
			resetState();
			state = STATESCAN;
		//SCANNING
		}else if (state == STATESCAN){
			if(checkIncomingCmd() == 1){
				if(checkPCCmd() == 1){
					sendPCStart();
					state = STATEINITIATE;
				}
			}
			if(packData() == 1){
				usb_rawhid_send(data_out, 15);
			}
			if(rotation == MAXROTATION){
				state = STATEEND; 
			}
		//scan complete cleanup and reset to state 0
		}else if (state == STATEEND){
			resetState();
			sendPCEnd();
			state = STATEIDLE;
		}//STATEEND
	}//main loop
}//main


