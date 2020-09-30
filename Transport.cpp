#include <Arduino.h>

#include "DIAG.h"
#include "Transport.h"

DCCEXParser * Transport::transportParser=0;

void Transport::parse(Print * stream,  byte * command, bool blocking) {
    DIAG(F("Transport parsing:      [%e]\n"), command);
     if (!transportParser) transportParser=new DCCEXParser();
     
    if (command[0]=='<') {
        transportParser->parse(stream,command,blocking);
    }
}
