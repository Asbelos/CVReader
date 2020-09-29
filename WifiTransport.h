#ifndef WifiTransport_h
#define WifiTransport_h

#include <Arduino.h>
#include <WiFiEspAT.h>

#include "Singelton.h"
#include "Transport.h"


class WifiTransport: public Singleton<WifiTransport>, public Transport
{
    friend WifiTransport* Singleton<WifiTransport>::get();
    friend void Singleton<WifiTransport>::kill();

private:

    WifiTransport (const WifiTransport&){};

    IPAddress dnsip;
    IPAddress ip;
    static WiFiServer server;
    WiFiUDP Udp;
    protocolType p;
    WiFiClient clients[MAX_SOCK_NUM]; 

public:
  
   void tcpHandler();
   void udpHandler();

    uint8_t setup();
    void loop();

    WifiTransport(/* args */);
    ~WifiTransport();
};

#endif // !WifiTransport_h