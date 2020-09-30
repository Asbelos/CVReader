#include <Arduino.h>

#include "DIAG.h"
#include "NetworkInterface.h"
#include "WifiTransport.h"
#include "EthernetTransport.h"

Transport* NetworkInterface::transport;

void NetworkInterface::setup(transportType tt, protocolType pt, uint16_t lp)
{
    uint8_t ok = 0;

    DIAG(F("\n[%s] Transport Setup In Progress ...\n"), tt ? "Ethernet" : "Wifi");

    switch (tt)
    {
    case WIFI:
    {
        transport = Singleton<WifiTransport>::get();
        ok = 1;
        break;
    };
    case ETHERNET:
    {
        transport = Singleton<EthernetTransport>::get();
        ok = 1;
        break;
    };
    default:
    {
        DIAG(F("\nERROR: Unknown Transport"));// Something went wrong
        break;
    }
    }

    if(ok) {  // configure the Transport and get it up and running
        transport->port = lp;
        transport->protocol = pt;
        // ok = transport->setup(pt,lp);
        ok = transport->setup();
    }

    DIAG(F("\n\n[%s] Transport %s ..."), tt ? "Ethernet" : "Wifi", ok ? "OK" : "Failed");
    
}

void NetworkInterface::setup(transportType tt, protocolType pt)
{
    NetworkInterface::setup(tt, pt, LISTEN_PORT);
}

void NetworkInterface::setup(transportType tt)
{
    NetworkInterface::setup(tt, TCP, LISTEN_PORT);
}

void NetworkInterface::setup()
{
    NetworkInterface::setup(ETHERNET, TCP, LISTEN_PORT);
}



void NetworkInterface::loop() {

    transport->loop();

}


NetworkInterface::NetworkInterface()
{
    // DIAG(F("NetworkInterface created "));
}

NetworkInterface::~NetworkInterface()
{
    // DIAG(F("NetworkInterface destroyed"));
}