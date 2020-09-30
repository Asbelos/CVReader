/*
 *  © 2020, Chris Harlow. All rights reserved.
 *  
 *  This is a basic, no frills CVreader example of a DCC++ compatible setup.
 *  There are more advanced examples in the examples folder i
 */
#include "Arduino.h"
#include "DCCEX.h"
#include "NetworkInterface.h"

DCCEXParser serialParser;

void setup()
{

  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. just in case
  }

  DIAG(F("\nNetwork Setup In Progress ...\n"));
  NetworkInterface::setup(ETHERNET, UDP, 8888); // specify WIFI or ETHERNET depending on if you have Wifi or an EthernetShield; Wifi has to be on Serial1 UDP or TCP for the protocol
  // NetworkInterface::setup(WIFI, UDP);          // Setup without port will use the by default port 2560
  // NetworkInterface::setup(WIFI);               // setup without port and protocol will use by default TCP on port 2560
  // NetworkInterface::setup();                   // all defaults ETHERNET, TCP on port 2560
  DIAG(F("\nNetwork Setup done ..."));

  DCC::begin(STANDARD_MOTOR_SHIELD);

  Serial.println("\nReady for DCC Commands ...");
}

void loop()
{
  DCC::loop();
  NetworkInterface::loop(); // loop adapts automatically against UDP or TCP
  serialParser.loop(Serial);
}