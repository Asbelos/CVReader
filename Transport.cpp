#include <Arduino.h>

#include "DIAG.h"
// #include "DCCEXParser.h"

#include "StringFormatter.h"
#include "Transport.h"

// DCCEXParser transportParser;

void Transport::parse(Print * stream,  byte * command, bool blocking) {
    DIAG(F("DCC parsing:            [%s]\n"), command);
    // Do the DCC Stuff
        
    // echo back (as mock)
    StringFormatter::send(stream, F("reply to: %s"), command);
}