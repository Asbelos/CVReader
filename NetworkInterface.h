#ifndef NetworkInterface_h
#define NetworkInterface_h

#include <Arduino.h>

#include "Transport.h"

typedef enum {
    TCP,
    UDP
} protocolType;

typedef enum  {
    WIFI,                   // using an AT (Version >= V1.7) command enabled ESP8266 not to be used in conjunction with the WifiInterface though! not tested for conflicts
    ETHERNET                   // using the EthernetShield
} transportType;



class NetworkInterface
{
private:
    static  Transport*  transport;

public:
    static void setup(transportType t, protocolType p, uint16_t port);        // specific port nummber
    static void setup(transportType t, protocolType p);                       // uses default port number
    static void setup(transportType t);                                       // defaults for protocol/port 
    static void setup();                                                      // defaults for all as above plus CABLE (i.e. using EthernetShield ) as default

    static void loop();

    NetworkInterface();
    ~NetworkInterface();
};

#endif