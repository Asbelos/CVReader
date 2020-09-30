#ifndef Transport_h
#define Transport_h

#include <Arduino.h>
// #include "MemStream.h"

#define MAX_ETH_BUFFER 250
#define MAX_SOCK_NUM 8
#define LISTEN_PORT 2560 // default listen port for the server

class Transport
{
private:
    uint8_t test;

public:
    uint16_t port;
    uint8_t protocol;
    uint8_t buffer[MAX_ETH_BUFFER];
    char packetBuffer[MAX_ETH_BUFFER];
    // MemStream *streamer;
    bool connected;

    // uint8_t virtual setup(int p, uint16_t port);
    uint8_t virtual setup();
    void virtual loop();

    void parse(Print *stream, byte *command, bool blocking);
};

#endif // !Transport_h