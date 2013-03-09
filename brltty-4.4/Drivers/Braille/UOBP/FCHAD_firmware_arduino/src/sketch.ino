/*
 * FCHAD firmware.  Firmware for the arduino to control any FCHAD device.  
 *
 * Copyright (C) 2012 by Timothy Hobbs, Samuel Thibault
 *
 * This hardware project would not have been possible without the guidance
 * and support of the many good people at brmlab <brmlab.cz> as well as
 * material support in terms of office space and machinery used for soldering
 * and manufacture of electric components.
 *
 * The FCHAD software comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brmlab.cz/user/timthelion
 *
 * This software is maintained by Timothy Hobbs <timothyhobbs@seznam.cz>
 */
#include "sketch.h"

///////////////////////////////////////////////////
//Frame Handlers///////////////////////////////////
///////////////////////////////////////////////////
void (*frameHandlers
        [NUM_FRAME_TYPES]
        [MAX_NUM_FRAME_SUBTYPES])
         (unsigned int,unsigned char *);

void * getHandler
 (unsigned char frameType
 ,unsigned char frameSubType){
 if(frameType >= NUM_FRAME_TYPES)return NULL;
 if(frameSubType >= MAX_NUM_FRAME_SUBTYPES) return NULL;
 return (void *)frameHandlers[frameType][frameSubType];
}

void initializeFrameHandlers(){
 int i, j;
 for(i=0;i<NUM_FRAME_TYPES;i++)
  for(j=0;j<MAX_NUM_FRAME_SUBTYPES;j++)
   frameHandlers[i][j]=NULL;
 //All frames subtypes which are NULL or otherwise undefined are ignored.
 frameHandlers[0][0]=&sendInitializationFrame;
 frameHandlers[1][1]=&displayChar;
}

void handleFrame(
 unsigned int  length,
 unsigned char type,
 unsigned char subType,
 unsigned char * information,
 void * callerParameter)
 /*We ignore the callerParameter.
   It is not needed for FCHAD code.
   It is used in the BRLTTY driver.*/{
 void (*handler)(unsigned int,unsigned char *) =
  (void (*)(unsigned int, unsigned char*))getHandler(type,subType);
 if(handler)(handler)(length,information);
 free(information);
}

//////////////////////////////////////////
///Initialization Frame///////////////////
//////////////////////////////////////////
void sendInitializationFrame(unsigned int length,unsigned char * information){
  #ifdef RUNTESTS
  testSolenoids();
  #endif
  Serial.begin(9600);
  int i = 0;
  #define INITIALIZER_LENGTH 74
  unsigned int initializerLength=INITIALIZER_LENGTH;
  unsigned char initializer[INITIALIZER_LENGTH]={
    UUID0,
    UUID1,
    UUID2,
    UUID3,
    UUID4,
    UUID5,
    UUID6,
    UUID7,
    UUID8,
    UUID9,
    UUID10,
    UUID11,
    UUID12,
    UUID13,
    UUID14,
    UUID15,
    /*These two bytes are the number of standard capabilities*/
    0,
    2,
/*FCHAD cell*/
    /*capabilityID*/
    0,
    3,
    /*nodeID*/
    0,
    /*NUM PAIRS*/
    1,
    /*Pairing type: paired*/
    1,
    /*capability to be paired with: FCHAD_SENSORS*/
    0,
    4,
    /*node to be paired with*/
    0,
    /*info length*/
    0,
    13,
/*Punch force setting,
we ignore this for now.
So we give it no value.*/
    /*Range*/
    0,
    0,
    /*Default*/
    0,
    0,
    /*Persistant value*/
    0,
    0,
/*MIN_DISPLAY_TIME setting,
we set this to 50ms*/
    /*Range*/
    0,
    0,
    /*Default*/
    0,
    0,
    /*Persistant value*/
    0,
    0,
    /*number of dots*/
    dotCount,
/*FCHAD Sensors*/
    /*capabilityID*/
    0,
    4,
    /*nodeID*/
    0,
    /*NUM PAIRS*/
    1,
    /*Pairing type: paired*/
    1,
    /*capability to be paired with: FCHAD_CELL*/
    0,
    3,
    /*node to be paired with*/
    0,
    /*Length*/
    0,
    16,
  /*threashhold setting,
  we ignore this for now.
  So we give it no value.*/
    /*Range*/
    0,
    0,
    /*Default*/
    0,
    0,
    /*Persistant value*/
    0,
    0,
  /*portamento setting,
  Turned on by default.*/
    /*Range*/
    0,
    0,
    /*Default*/
    0,
    0,
    /*Persistant value*/
    0,
    0,
    /*Sensor rows*/
    0,
    1,
    /*Sensor columns*/
    0,
    NUM_SENSORS,
    /*Number of extended capabilities*/
    0,
    0};
  sendFrame(initializerLength,0,1,initializer,NULL);
}
//////////////////////////////////////////
///Display functions//////////////////////
//////////////////////////////////////////
void dot_display_init()
{
  for(int n = 0;n<dotCount;n++)
  {
    pinMode(dotPins[n], OUTPUT);
  }
}

void displayChar(uint16_t length, unsigned char * information)
{
  if(information[0]==0){ //This is the NODE ID.  We are node 0 as this firmware only supports one cell.
   uint16_t character = (uint16_t)information[1];
   character += (uint16_t)information[2]<<8;
   displayAChar(character);
  }
}

void displayAChar(uint16_t character){
 #ifdef DISPLAY
 for(uint16_t n = 0;n<dotCount;n++){
  if((1 << n) & character){
    digitalWrite(dotPins[n],HIGH);
  }else{
   digitalWrite(dotPins[n],LOW);
  }
 }
 #endif
}

void testSolenoids(){
 uint16_t character=0;
 for(int n = 0;n<dotCount;n++)
 {
  character+=1<<n;
  displayAChar(character);
  delay(1500);
 }
 displayAChar(0);
}
//////////////////////////////////////////////
////Send sensor touch signals/////////////////
//////////////////////////////////////////////
void sendOnDown(unsigned char sensor){
  unsigned char information [5] = {
  //Node
  0,
  //Row
  0,
  0,
  //Column
  0,
  sensor};
 sendFrame(5, 2, 3, information,NULL);
}
void sendOnUp(unsigned char sensor){
 unsigned char information [5] = {
  //Node
  0,
  //Row
  0,
  0,
  //Column
  0,
  sensor};
 sendFrame(5, 2, 4, information,NULL);
}


//////////////////////////////////////////////
////Standard initializers/////////////////////
//////////////////////////////////////////////
TouchSensors*touchSensors;
void setup()
{
  // initialize the serial communication:
  Serial.begin(SERIAL_BAUD);
  // initialize the the pins as outputs:
  dot_display_init();
  // initialize the touch sensors:
  touchSensors=setupTouch();
  initializeFrameHandlers();
}

void loop() {
 readTouchInputs(&sendOnDown,&sendOnUp,touchSensors);
 checkForFrameAndReact(&handleFrame,START_OF_FRAME,NULL,NULL);
}
