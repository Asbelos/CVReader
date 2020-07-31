/*
 *  © 2020, Chris Harlow. All rights reserved.
 *  
 *  This file is part of Asbelos DCC API
 *
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */



#include "DCC.h"
#include "DIAG.h"
#include "DCCEXParser.h"
#include "WifiInterface.h"

// this code is here to demonstrate use of the DCC API and other techniques

// myFilter is an example of an OPTIONAL command filter used to intercept < > commands from
// the usb or wifi streamm.  It demonstrates how a command may be intercepted
//  or even a new command created without having to break open the API library code.
// The filter is permitted to use or modify the parameter list before passing it on to 
// the standard parser. By setting the opcode to 0, the standard parser will 
// just ignore the command on the assumption that you have already handled it.
//
// The filter must be enabled by calling the DCC EXParser::setFilter method, see use in setup().
 
void myFilter(Print & stream, byte & opcode, byte & paramCount, int p[]) {
    (void)stream; // avoid compiler warning if we don't access this parameter
    switch (opcode) {  
       case '!': // Create a bespoke new command to clear all loco reminders <!> or specific locos e.g <! 3 4 99>
             if (paramCount==0) DCC::forgetAllLocos();
             else for (int i=0;i<paramCount;i++) DCC::forgetLoco(p[i]);            
             opcode=0;  // tell parser to ignore this command as we have done it already
             break; 
       default:  // drop through and parser will use the command unaltered.   
            break;  
    }
}

// Callback functions are necessary if you call any API that must wait for a response from the 
// programming track. The API must return immediately otherwise other loop() functions would be blocked.
// Your callback function will be invoked when the data arrives from the prog track.
// See the DCC:getLocoId example in the setup function. 


void myCallback(int result) {
  DIAG(F("\n getting Loco Id callback result=%d"),result); 
}


// Create a serial command parser... This is OPTIONAL if you don't need to handle JMRI type commands
// from the Serial port.
DCCEXParser  serialParser;


// Try monitoring the memory
#include "freeMemory.h"
int ramLowWatermark=32767; // This figure gets overwritten dynamically in loop() 

void setup() {

  // The main sketch has responsibilities during setup()
  
  // Responsibility 1: Start the usb connection for diagnostics and possible JMRI input
  // DIAGSERAL is normally Serial but uses SerialUSB on a SAMD processor
   DIAGSERIAL.begin(115200);
   while(!DIAGSERIAL);
   
   // Responsibility 2: Start the DCC engine.   
   DCC::begin();

   // Responsibility 3: Optionally Start the WiFi interface if required.
   //   NOTE: On a Uno you will have to provide a SoftwareSerial 
   //         configured for the pins connected to the Wifi card
   //         and a 9600 baud rate. 
   //  setup(serial, F(router name), F(password) , port)
   //            
#ifdef WIFI
    Serial1.begin(115200);
    WifiInterface::setup(Serial1, F(WIFI_CONNECT_TO_SSID), F(WIFI_CONNECT_PASSWORD),F("DCCEX"),F("CVReader"),3532); // (3532 is 0xDCC decimal... )
#endif
    
   //  This is just for demonstration purposes 
   DIAG(F("\n===== CVReader demonstrating DCC::getLocoId() call ==========\n"));
   DCC::getLocoId(myCallback); // myCallback will be called with the result 
   DIAG(F("\n===== DCC::getLocoId has returned, but the callback wont be executed until we are in loop() ======\n"));
   
   // Optionally tell the command parser to use my example filter.
   // This will intercept JMRI commands from both USB and Wifi 
   DCCEXParser::setFilter(myFilter);
   
   DIAG(F("\nReady for JMRI commands\n"));
   
}

void loop() {      
  // The main sketch has responsibilities during loop()
  
  // Responsibility 1: Handle DCC background processes
  //                   (loco reminders and power checks)
  DCC::loop(); 

  // Responsibility 2: handle any incoming commands on USB connection
  serialParser.loop(DIAGSERIAL);

  // Responsibility 3: Optionally handle any incoming WiFi traffic
#ifdef WIFI
  WifiInterface::loop(Serial1);
#endif

  // Your additional code
  
  // OPtionally report any decrease in memory (will automatically trigger on first call)
  int freeNow=freeMemory();
  if (freeNow<ramLowWatermark) {
    ramLowWatermark=freeNow;
    DIAG(F("\nFree RAM=%d\n"),ramLowWatermark);
  }
}
