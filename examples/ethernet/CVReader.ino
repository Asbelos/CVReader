/*
 *  © 2020, Chris Harlow. All rights reserved.
 *  
 *  This is a basic, no frills CVreader example of a DCC++ compatible setup.
 *  There are more advanced examples in the examples folder i
 */
#include "Arduino.h"
#include "DCCEX.h"
#include "EthernetInterface.h"


DCCEXParser  serialParser;

void setup() {
  
  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. just in case
  }
  Serial.println("Starting ...");
  EthernetInterface::setup(TCP, 8888); // UDP or TCP are possible
  // EthernetInterface::setup(UDP, 8888); // UDP or TCP are possible
  DCC::begin(STANDARD_MOTOR_SHIELD);  
  
  Serial.println("\nReady for DCC Commands ..."); 
}

void loop() {      
  DCC::loop(); 
  EthernetInterface::loop();         // loop adapts automatically against UDP or TCP
  serialParser.loop(Serial);
}