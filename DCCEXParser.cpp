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
#include "StringFormatter.h"
#include "DCCEXParser.h"
#include "DCC.h"
#include "DCCWaveform.h"
#include "Turnouts.h"
#include "Outputs.h"
#include "Sensors.h"

#include "EEStore.h"
#include "DIAG.h"

const char VERSION[] PROGMEM ="99.666";

const int HASH_KEYWORD_PROG=-29718;
const int HASH_KEYWORD_MAIN=11339;

int DCCEXParser::stashP[MAX_PARAMS];
bool DCCEXParser::stashBusy;
 
 Print & DCCEXParser::stashStream=Serial;  // keep compiler happy but ovevride in constructor

// This is a JMRI command parser, one instance per incoming stream
// It doesnt know how the string got here, nor how it gets back.
// It knows nothing about hardware or tracks... it just parses strings and
// calls the corresponding DCC api.
// Non-DCC things like turnouts, pins and sensors are handled in additional JMRI interface classes. 

DCCEXParser::DCCEXParser() {}
void DCCEXParser::flush() {
DIAG(F("\nBuffer flush"));
      bufferLength=0;
      inCommandPayload=false;   
}

void DCCEXParser::loop(Stream & stream) {
   while(stream.available()) {
    if (bufferLength==MAX_BUFFER) {
       flush();
    }
    char ch = stream.read();
    if (ch == '<') {
      inCommandPayload = true;
      bufferLength=0;
      buffer[0]='\0';
    } 
    else if (ch == '>') {
      buffer[bufferLength]='\0';
      parse( stream, buffer, false); // Parse this allowing async responses
      inCommandPayload = false;
      break;
    } else if(inCommandPayload) {
      buffer[bufferLength++]= ch;
    }
  }
  }

 int DCCEXParser::splitValues( int result[MAX_PARAMS], const byte * cmd) {
  byte state=1;
  byte parameterCount=0;
  int runningValue=0;
  const byte * remainingCmd=cmd+1;  // skips the opcode
  bool signNegative=false;
  
  // clear all parameters in case not enough found
  for (int i=0;i<MAX_PARAMS;i++) result[i]=0;
  
  while(parameterCount<MAX_PARAMS) {
      byte hot=*remainingCmd;
      
       switch (state) {
    
            case 1: // skipping spaces before a param
               if (hot==' ') break;
               if (hot == '\0' || hot=='>') return parameterCount;
               state=2;
               continue;
               
            case 2: // checking sign
               signNegative=false;
               runningValue=0;
               state=3; 
               if (hot!='-') continue; 
               signNegative=true;
               break; 
            case 3: // building a parameter   
               if (hot>='0' && hot<='9') {
                   runningValue=10*runningValue+(hot-'0');
                   break;
               }
               if (hot>='A' && hot<='Z') {
                   // Since JMRI got modified to send keywords in some rare cases, we need this
                   // Super Kluge to turn keywords into a hash value that can be recognised later
                   runningValue = ((runningValue << 5) + runningValue) ^ hot;
                   break;
               }
               result[parameterCount] = runningValue * (signNegative ?-1:1);
               parameterCount++;
               state=1; 
               continue;
         }   
         remainingCmd++;
      }
      return parameterCount;
}

FILTER_CALLBACK  DCCEXParser::filterCallback=0;
void DCCEXParser::setFilter(FILTER_CALLBACK filter) {
  filterCallback=filter;  
}
   
// See documentation on DCC class for info on this section
void DCCEXParser::parse(Print & stream, const byte *com, bool banAsync) {
    DIAG(F("\nPARSING:%s\n"),com);
    asyncBanned=banAsync;
    (void) EEPROM; // tell compiler not to warn thi is unused
    int p[MAX_PARAMS]; 
    while (com[0]=='<' || com[0]==' ') com++; // strip off any number of < or spaces
    byte params=splitValues(p, com); 
    byte opcode=com[0];
     
    if (filterCallback)  filterCallback(stream,opcode,params,p);
     
    // Functions return from this switch if complete, break from switch implies error <X> to send
    switch(opcode) {
    case '\0': return;    // filterCallback asked us to ignore 
    case 't':       // THROTTLE <t REGISTER CAB SPEED DIRECTION>
        DCC::setThrottle(p[1],p[2],p[3]);
        StringFormatter::send(stream,F("<T %d %d %d>"), p[0], p[2],p[3]);
        return;
    
    case 'f':       // FUNCTION <f CAB BYTE1 [BYTE2]>
        if (parsef(stream,params,p)) return;
        break;
         
    case 'a':       // ACCESSORY <a ADDRESS SUBADDRESS ACTIVATE>
        if(p[2] != (p[2] & 1)) return;
        DCC::setAccessory(p[0],p[1],p[2]==1);
        return;

    case 'T':       // TURNOUT  <T ...> 
        if (parseT(stream,params,p)) return;
        break;

    case 'Z':       // OUTPUT <Z ...> 
      if (parseZ(stream,params,p)) return; 
      break; 

    case 'S':        // SENSOR <S ...> 
      if (parseS(stream,params,p)) return; 
      break;
    
    case 'w':      // WRITE CV on MAIN <w CAB CV VALUE>
        DCC::writeCVByteMain(p[0],p[1],p[2]);
        return;

    case 'b':      // WRITE CV BIT ON MAIN <b CAB CV BIT VALUE>   
        DCC::writeCVBitMain(p[0],p[1],p[2],p[3]);
        return;

    case 'W':      // WRITE CV ON PROG <W CV VALUE CALLBACKNUM CALLBACKSUB>
        if (!stashCallback(stream,p)) break;
        DCC::writeCVByte(p[0],p[1],callback_W);
        return;

    case 'B':      // WRITE CV BIT ON PROG <B CV BIT VALUE CALLBACKNUM CALLBACKSUB>
        if (!stashCallback(stream,p)) break; 
        DCC::writeCVBit(p[0],p[1],p[2],callback_B);
        return;
        
    case 'R':     // READ CV ON PROG <R CV CALLBACKNUM CALLBACKSUB>
        if (!stashCallback(stream,p)) break;
        DCC::readCV(p[0],callback_R);
        return;

    case '1':      // POWERON <1   [MAIN|PROG]>
    case '0':      // POWEROFF <0 [MAIN | PROG] >
        if (params>1) break;
        {
          POWERMODE mode= opcode=='1'?POWERMODE::ON:POWERMODE::OFF;
          if (params==0) {
              DCCWaveform::mainTrack.setPowerMode(mode);
              DCCWaveform::progTrack.setPowerMode(mode);
              StringFormatter::send(stream,F("<p%c>"),opcode);
              return;
          }
          if (p[0]==HASH_KEYWORD_MAIN) {
              DCCWaveform::mainTrack.setPowerMode(mode);
              StringFormatter::send(stream,F("<p%c MAIN>"),opcode);
              return;
          }
          if (p[0]==HASH_KEYWORD_PROG) {
              DCCWaveform::progTrack.setPowerMode(mode);
              StringFormatter::send(stream,F("<p%c PROG>"),opcode);
              return;
          }
          DIAG(F("keyword hash=%d\n"),p[0]);
          break;
        }
        return;      
     
    case 'c':     // READ CURRENT <c>
        StringFormatter::send(stream,F("<a %d>"), (int)(MAIN_SENSE_FACTOR * DCCWaveform::mainTrack.getLastCurrent()));
        return;

    case 'Q':         // SENSORS <Q>
        Sensor::status(stream);
        break;

    case 's':      // <s>
        StringFormatter::send(stream,F("<p%d>"),DCCWaveform::mainTrack.getPowerMode()==POWERMODE::ON );
        StringFormatter::send(stream,F("<iDCC-Asbelos BASE STATION FOR ARDUINO / %S: V-%S %s/%s>"), BOARD_NAME, VERSION, __DATE__, __TIME__ );
        // TODO Send stats of  speed reminders table 
        // TODO send status of turnouts etc etc 
        return;

    case 'E':     // STORE EPROM <E>
        EEStore::store();
        StringFormatter::send(stream,F("<e %d %d %d>"), EEStore::eeStore->data.nTurnouts, EEStore::eeStore->data.nSensors, EEStore::eeStore->data.nOutputs);
        return;

    case 'e':     // CLEAR EPROM <e>
        EEStore::clear();
        StringFormatter::send(stream, F("<O>"));
        return;

     case ' ':     // < >
        StringFormatter::send(stream,F("\n"));
        return;
        
     case 'D':     // < >
        DCC::setDebug(p[0]==1);
        DIAG(F("\nDCC DEBUG MODE %d"),p[0]==1);
        return;

    case '#':     // NUMBER OF LOCOSLOTS <#>
	StringFormatter::send(stream,F("<# %d>"), MAX_LOCOS);
	return;
        
     default:  //anything else will drop out to <X>
        break;
    
    } // end of opcode switch 

    // Any fallout here sends an <X>
       StringFormatter::send(stream, F("<X>"));
}

bool DCCEXParser::parseZ( Print & stream,int params, int p[]){
      
        
    switch (params) {
        
        case 2:                     // <Z ID ACTIVATE>
           {
            Output *  o=Output::get(p[0]);      
            if(o==NULL) return false;
            o->activate(p[1]);
            StringFormatter::send(stream,F("<Y %d %d>"), p[0],p[1]);
           }
            return true;

        case 3:                     // <Z ID PIN INVERT>
            Output::create(p[0],p[1],p[2],1);
            return true;

        case 1:                     // <Z ID>
            return Output::remove(p[0]);
            
        case 0:                    // <Z>
            return Output::showAll(stream); 

         default:
             return false; 
        }
    }    

//===================================
bool DCCEXParser::parsef(Print & stream, int params, int p[]) {
    // JMRI sends this info in DCC message format but it's not exactly 
    //      convenient for other processing
    if (params==2) {
      byte groupcode=p[1] & 0xE0;
      if (groupcode == 0x80) {
        byte normalized= (p[1]<<1 & 0x1e ) | (p[1]>>4 & 0x01);
        funcmap(p[0],normalized,0,4);
      }
      else if (groupcode == 0xC0) {
        funcmap(p[0],p[1],5,8);
      }
      else if (groupcode == 0xA0) {
        funcmap(p[0],p[1],9,12);
      }
    }   
    if (params==3) { 
      if (p[1]==222) funcmap(p[0],p[2],13,20);
      else if (p[1]==223) funcmap(p[0],p[2],21,28);
    }
    (void)stream;// NO RESPONSE    
   return true;
}

void DCCEXParser::funcmap(int cab, byte value, byte fstart, byte fstop) {
   for (int i=fstart;i<=fstop;i++) {
    DCC::setFn(cab, i, value & 1);
    value>>=1; 
   }
}

//===================================
bool DCCEXParser::parseT(Print & stream, int params, int p[]) {
  switch(params){
        case 0:                    // <T>
            return (Turnout::showAll(stream));             break;

        case 1:                     // <T id>
            if (!Turnout::remove(p[0])) return false;
            StringFormatter::send(stream,F("<O>"));
            return true;

        case 2:                     // <T id 0|1>
             if (!Turnout::activate(p[0],p[1])) return false;
             Turnout::show(stream,p[0]);
            return true;

        case 3:                     // <T id addr subaddr>
            if (!Turnout::create(p[0],p[1],p[2])) return false;
            StringFormatter::send(stream,F("<O>"));
            return true;

        default:
            return false; // will <x>                    
        }
}

bool DCCEXParser::parseS( Print & stream,int params, int p[]) {
     
        switch(params){

        case 3:                     // argument is string with id number of sensor followed by a pin number and pullUp indicator (0=LOW/1=HIGH)
            Sensor::create(p[0],p[1],p[2]);
            return true;

        case 1:                     // argument is a string with id number only
            if (Sensor::remove(p[0])) return true;
            break;

        case -1:                    // no arguments
            Sensor::show(stream);
            return true;

        default:                     // invalid number of arguments
            break;
        }
    return false;
}


 // CALLBACKS must be static 
bool DCCEXParser::stashCallback(Print & stream,int p[MAX_PARAMS]) {
       if (stashBusy || asyncBanned) return false;
       stashBusy=true; 
      stashStream=stream;
      memcpy(stashP,p,MAX_PARAMS*sizeof(p[0]));
      return true;
 }       
 void DCCEXParser::callback_W(int result) {
        StringFormatter::send(stashStream,F("<r%d|%d|%d %d>"), stashP[2], stashP[3],stashP[0],result==1?stashP[1]:-1);
        stashBusy=false;
 }  
 
void DCCEXParser::callback_B(int result) {        
        StringFormatter::send(stashStream,F("<r%d|%d|%d %d %d>"), stashP[3],stashP[4], stashP[0],stashP[1],result==1?stashP[2]:-1);
        stashBusy=false;
}
void DCCEXParser::callback_R(int result) {        
        StringFormatter::send(stashStream,F("<r%d|%d|%d %d>"),stashP[1],stashP[2],stashP[0],result);
        stashBusy=false;
}
       
