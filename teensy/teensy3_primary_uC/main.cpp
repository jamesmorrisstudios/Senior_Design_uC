#include "main.h"

int rotation = MAXROTATION/2;
char data[BUFSIZE] = {0};
//
//byteBuffer holds the incoming scan data (480 16 bit chunks)
//inByte counts which byte is currently being added to from the FPGA
//outByte says what byte is next in line to send
//data is sent when inByte - outByte >= BUFSIZE
//
volatile byte byteBuffer[MAXHEIGHT*2+20];
volatile int inByte = 0;
volatile int outByte = 0;
volatile byte buffer[2];


//
//Clears the PC transmit and recieve data buffer
//
void clearDataBuffer(){
	for(int i=0;i<BUFSIZE;i++){
		data[i] = 0;
	}
}

//
//Checks if there is an incoming command from the PC
//Return: 1 if there is a command, -1 otherwise
//
int checkIncomingCmd() {
	if(usb_rawhid_available() >= BUFSIZE){
		return 1;
	}else{
		return -1;
	}
}

//
//Checks what the command is from the PC
//Return 1 to start scan, 2 to end scan, -1 for error
//
int checkPCCmd() {
	byte cmd;
	usb_rawhid_recv(data, 15);
	cmd = data[0] >> 6;
	if(cmd == 1){
		return 1;
	}else if (cmd == 2){
		return 2;
	}
	return -1;
}

//
//Sends the start scan signal to the PC
//Also used to confirm starting a scan
//
void sendPCStart() {
	data[0] = CMDSTART;
	usb_rawhid_send(data, 15);
}

//
//Sends the End scan signal to the PC
//
void sendPCEnd() {
	data[0] = CMDEND;
	usb_rawhid_send(data, 15);
}

//
//Checks if there is enough data to send and if so transmits it to the PC
//
void transmitData(){
	//if there are at least 64 bytes of unsent data or all inBytes recieved then transmit
	if(inByte - outByte >= BUFSIZE || inByte == MAXHEIGHT*2){
		packData();
		usb_rawhid_send(data, 15);
		//once done transmitting the data take another picture
		if(outByte >= MAXHEIGHT*2){
			rotate(STEP_DIR_RIGHT);
			takePicture();
		}
	}
}

//
//Packs the data from the incoming (from FPGA) buffer into the PC transmission buffer
//
void packData() {
	//Loop over over the size of the data packet and break if the end of the source data is reached
	for(int i=0;i<BUFSIZE/PACKETSIZE;i++) {
		data[0+i*PACKETSIZE] = byteBuffer[0+outByte];
		data[1+i*PACKETSIZE] = byteBuffer[1+outByte];
		outByte += 2;
	}
}

//
//Resets the scan state for when returning to Idle mode
//
void resetState() {
	rotateStepperOrigin();
}

//
//Tells the stepper to rotate one step in the given direction
//blocks until the stepper has moved
//
void rotate(int dir) {
	//Set the direction and wait for it to set
	digitalWriteFast(PIN_STEP_DIR, dir);
	delayMicroseconds(10);
	//Send the step command pulse
	digitalWriteFast(PIN_STEP_GO, HIGH);
	delayMicroseconds(10);
	digitalWriteFast(PIN_STEP_GO, LOW);
	//Wait long enough for the stepper to rotate
	//delay(100); //TEST
	if(dir){
		rotation++;
	}else{
		rotation--;
	}
}

//
//Rotates the stepper motor back to the origin.
//Origin is defined as where the stepper pointed at power on
//
void rotateStepperOrigin() {
	if(rotation > MAXROTATION/2){
		while(rotation > MAXROTATION/2){
			rotate(STEP_DIR_LEFT);
			rotation--;
		}
	}else{
		while(rotation < MAXROTATION/2){
			rotate(STEP_DIR_RIGHT);
			rotation++;
		}
	}
}

//
//Rotates the stepper motor to the scan start location (45 degrees left of the origin)
//
void rotateStepperStart() {
	while(rotation > 0){
		rotate(STEP_DIR_LEFT);
		rotation--;
	}
}

//
//signal the FPGA to take a picture
//clears byte counters for the FPGA FIFO
//
void takePicture() {
	inByte = 0;
	outByte = 0;
	digitalWriteFast(PIN_SPI_SS, LOW);
	delayMicroseconds(10);
	digitalWriteFast(PIN_SPI_SS, HIGH);
}

//
//ISR for recieving data from the FPGA
//
void recieveValue() {
	buffer[0] = 3; //TEST
	buffer[1] = 232; //TEST

	//spiRec(buffer, 2); //TEST uncomment for normal operation
	byteBuffer[inByte] = buffer[0];
	byteBuffer[inByte+1] = buffer[1];
	inByte += 2;
}

//
//Sets the camera control state
//
void initCamera(){



}

//
//Sets all control pin input/output and default states
//does not include any pins related to SPI
//
void initControlPins() {

	pinMode(PIN_STEP_DIR, OUTPUT);
	digitalWriteFast(PIN_STEP_DIR, LOW);
	
	pinMode(PIN_STEP_GO, OUTPUT);
	digitalWriteFast(PIN_STEP_GO, LOW);
	
	pinMode(PIN_STEP_SLEEP, OUTPUT);
	digitalWriteFast(PIN_STEP_SLEEP, STEP_SLEEP);
	
	pinMode(PIN_LASER, OUTPUT);
	digitalWriteFast(PIN_LASER, LASER_OFF);
}

//
//Sets the SPI configuration and pin states
//
void setupSPI() {

	pinMode(PIN_SPI_CTRL, INPUT);
	attachInterrupt(PIN_SPI_CTRL, recieveValue, FALLING);
	
	pinMode(PIN_SPI_SS, OUTPUT);
	digitalWriteFast(PIN_SPI_SS, HIGH);
	
	spiInit(12);
}

//
//Main program control point
//Handles state management
//
extern "C" int main(void) {
	//state == 0: IDLE wait for scan start signal
	//state == 1: INITIATE SCAN: reset all data and start new scan
	//state == 2; scanning... transmitting data
	//state == 3: scan complete cleanup and reset to state 0
	int state = STATEIDLE;

	initControlPins();
	setupSPI();
	
	while(1){ //Main loop
		clearDataBuffer();
	
		//IDLE
		if (state == STATEIDLE){
			if(checkIncomingCmd() == 1){
				if(checkPCCmd() == 1){
					state = STATEINITIATE;
				}
			}
			
		//INITIATE
		}else if(state == STATEINITIATE){
			resetState();
			rotateStepperStart();
			sendPCStart();
			takePicture();
			state = STATESCAN;
			
		//SCANNING
		}else if (state == STATESCAN){
		
			recieveValue();//TEST this normally is by ISR
		
			//Check if the PC sent a command to either restart
			//or cancel the scan
			if(checkIncomingCmd() == 1){
				int cmd = checkPCCmd();
				if(cmd == 1){
					state = STATEINITIATE;
				}else if(cmd == 2){
					state = STATEEND;
				}
			}
			//Send any data that we can
			transmitData();
			//End the scan if we completed rotating
			if(rotation == MAXROTATION){
				state = STATEEND; 
			}
			
		//SCANEND
		}else if (state == STATEEND){
			resetState();
			sendPCEnd();
			state = STATEIDLE;
		}
		
	}//main loop
}//main


