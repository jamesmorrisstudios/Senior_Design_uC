#include "WProgram.h"
#define USB_RAWHID
#include "usb_rawhid.h"
#include "nativeSPI.h"
#include "Wire.h"
#include "mk20dx128.h"


#define BUFSIZE 64
#define PACKETSIZE 2
#define MAXHEIGHT 480
#define MAXROTATION 200 //Changed from 180 to match stepper motor with half stepping enabled

#define CMDSTART 64
#define CMDEND 128

#define STATEIDLE 0
#define STATEINITIATE 1
#define STATESCAN 2
#define STATEEND 3

#define nop asm volatile ("nop\n\t")

//Pin number definitions
#define PIN_STEP_DIR 0
#define PIN_STEP_GO 1
#define PIN_STEP_SLEEP 2
#define PIN_LASER 3
#define PIN_SPI_MISO 12
#define PIN_SPI_MOSI 11
#define PIN_SPI_SS 10
#define PIN_SPI_CTRL 9

//Pin state definitions
#define STEP_DIR_LEFT LOW
#define STEP_DIR_RIGHT !STEP_DIR_LEFT
#define STEP_SLEEP LOW
#define STEP_WAKE !STEP_SLEEP

#define LASER_OFF LOW
#define LASER_ON !LASER_OFF


//Function declarations
void clearDataBuffer();
void setupSPI();
void initControlPins();
int checkIncomingCmd();
int checkPCCmd();
void sendPCStart();
void sendPCEnd();
void resetState();
void takePicture();
void rotate(int dir);
void rotateStepperOrigin();
void rotateStepperStart();
void packData();
void transmitData();
void initCamera();