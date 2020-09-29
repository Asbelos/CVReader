#ifndef EthernetTransport_h
#define EthernetTransport_h
#include <Arduino.h>
#include <Ethernet.h>

#include "Singelton.h"
#include "Transport.h"


/* some generated mac addresses as EthernetShields don't have one by default in HW.
 * Sometimes they come on a sticker on the EthernetShield then use this address otherwise
 * just choose one from below or generate one yourself. Only condition is that there is no 
 * other device on your network with the same Mac address.
 * 
 * 52:b8:8a:8e:ce:21
 * e3:e9:73:e1:db:0d
 * 54:2b:13:52:ac:0c
 * c2:d8:d4:7d:7c:cb
 * 86:cf:fa:9f:07:79
 */

/**
 * @brief Network Configuration
 * 
 */
#define MAC_ADDRESS                        \
    {                                      \
        0x52, 0xB8, 0x8A, 0x8E, 0xCE, 0x21 \
    }                            // MAC address of your networking card found on the sticker on your card or take one from above
#define IP_ADDRESS 10, 0, 0, 101 // Just in case we don't get an adress from DHCP try a static one;

class EthernetTransport : public Singleton<EthernetTransport>, public Transport
{
    friend EthernetTransport* Singleton<EthernetTransport>::get();
    friend void Singleton<EthernetTransport>::kill();

private:

    EthernetTransport (const EthernetTransport&){};

    IPAddress dnsip;
    IPAddress ip;
    static EthernetServer server;
    EthernetUDP Udp;
    protocolType p;
    EthernetClient clients[MAX_SOCK_NUM];
    uint8_t mac[6] = MAC_ADDRESS;

public:
    void tcpHandler();
    void udpHandler();


    uint8_t setup();
    void virtual loop();

    EthernetTransport();
    ~EthernetTransport();
};

#endif // !EthernetTransport_h