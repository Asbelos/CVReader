#include <Arduino.h>
#include <WiFiEspAT.h>

#include "DIAG.h"
#include "StringFormatter.h"

#include "NetworkInterface.h"
#include "WifiTransport.h"

// Emulate Serial1 on pins 6/7 if not present
#if defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
#include <SoftwareSerial.h>
SoftwareSerial Serial1(6, 7); // RX, TX
#define AT_BAUD_RATE 9600
#else
#define AT_BAUD_RATE 115200
#endif

WiFiServer WifiTransport::server = WiFiServer(LISTEN_PORT);

void WifiTransport::udpHandler()
{
    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
        DIAG(F("\nReceived packet of size:[%d]\n"), packetSize);
        IPAddress remote = Udp.remoteIP();
        DIAG(F("From:                   [%d.%d.%d.%d:"), remote[0], remote[1], remote[2], remote[3]);
        char portBuffer[6];
        DIAG(F("%s]\n"), utoa(Udp.remotePort(), portBuffer, 10)); // DIAG has issues with unsigend int's so go through utoa

        // read the packet into packetBufffer
        Udp.read(packetBuffer, MAX_ETH_BUFFER);
        // terminate buffer properly
        packetBuffer[packetSize]='\0';

        DIAG(F("Command:                [%s]\n"), packetBuffer);
        // execute the command via the parser
        
        // check if we have a response if yes then 
        
        // send the reply 
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        parse(&Udp, (byte *)packetBuffer, true);
        Udp.endPacket();

        /*
        streamer->flush();

        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());

        // ethParser.parse(streamer, (byte *)packetBuffer, true); // set to true so it is sync cf. WifiInterface

        if (streamer->available() == 0)
        {
            DIAG(F("\nNo response\n"));
        }
        else
        {
            // send the reply
            DIAG(F("Response: %s\n"), (char *)buffer);
            Udp.write((char *)buffer);
            Udp.endPacket();
        }
        */
       
        // clear out the PacketBuffer
        memset(packetBuffer, 0, MAX_ETH_BUFFER); // reset PacktBuffer
        return;
    }
}

void WifiTransport::tcpHandler()
{
    // get client from the server
    WiFiClient client = server.accept();

    // check for new client
    if (client)
    {
        for (byte i = 0; i < MAX_SOCK_NUM; i++)
        {
            if (!clients[i])
            {
                // On accept() the EthernetServer doesn't track the client anymore
                // so we store it in our client array
                clients[i] = client;
                break;
            }
        }
    }
    // check for incoming data from all possible clients
    for (byte i = 0; i < MAX_SOCK_NUM; i++)
    {
        if (clients[i] && clients[i].available() > 0)
        {
            // read bytes from a client
            int count = clients[i].read(buffer, MAX_ETH_BUFFER);
            IPAddress remote = clients[i].remoteIP();
            buffer[count] = '\0'; // terminate the string properly
            DIAG(F("\nReceived packet of size:[%d] from [%d.%d.%d.%d]\n"), count, remote[0], remote[1], remote[2], remote[3]);
            DIAG(F("Client #:               [%d]\n"), i);
            DIAG(F("Command:                [%s]\n"), buffer);

            parse(&(clients[i]), buffer, true);

            /*
            // as we use buffer for recv and send we have to reset the write position
            streamer->setBufferContentPosition(0, 0);

            ethParser.parse(streamer, buffer, true); // set to true to that the execution in DCC is sync

            if (streamer->available() == 0)
            {
                DIAG(F("No response\n"));
            }
            else
            {
                buffer[streamer->available()] = '\0'; // mark end of buffer, so it can be used as a string later
                DIAG(F("Response: %s\n"), (char *)buffer);
                if (clients[i].connected())
                {
                    clients[i].write(buffer, streamer->available());
                }
            }
            */
        }
        // stop any clients which disconnect
        for (byte i = 0; i < MAX_SOCK_NUM; i++)
        {
            if (clients[i] && !clients[i].connected())
            {
                DIAG(F("Disconnect client #%d \n"), i);
                clients[i].stop();
            }
        }
    }
}

uint8_t WifiTransport::setup()
{

    p = (protocolType)protocol;

    Serial1.begin(AT_BAUD_RATE);
    WiFi.init(Serial1);

    if (WiFi.status() == WL_NO_MODULE)
    {
        DIAG(F("Communication with WiFi module failed!\n"));
        return 0;
    }

    DIAG(F("Waiting for connection to WiFi "));
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        DIAG(F("."));
    }

    // Setup the protocol handler
    DIAG(F("\n\nNetwork Protocol:      [%s]"), p ? "UDP" : "TCP");

    switch (p)
    {
    case UDP:
    {
        if (Udp.begin(port))
        {
            connected = true;
            ip = WiFi.localIP();
        }
        else
        {
            DIAG(F("\nUDP client failed to start"));
            connected = false;
        }
        break;
    };
    case TCP:
    {
        server = WiFiServer(port);
        server.begin();
        connected = true;
        ip = WiFi.localIP();
        break;
    };
    default:
    {
        DIAG(F("Unkown Ethernet protocol; Setup failed"));
        connected = false;
        break;
    }
    }

    if (connected)
    {
        DIAG(F("\nLocal IP address:      [%d.%d.%d.%d]"), ip[0], ip[1], ip[2], ip[3]);
        DIAG(F("\nListening on port:     [%d]"), port);
        dnsip = WiFi.dnsServer1();
        DIAG(F("\nDNS server IP address: [%d.%d.%d.%d] "), dnsip[0], dnsip[1], dnsip[2], dnsip[3]);
        return 1;
    }
    // something went wrong
    return 0;
}

void WifiTransport::loop()
{

    switch (p)
    {
    case UDP:
    {
        udpHandler();
        break;
    };
    case TCP:
    {
        tcpHandler();
        break;
    };
    }
}

WifiTransport::WifiTransport()
{
    // DIAG(F("WifiTransport created "));
}

WifiTransport::~WifiTransport()
{
    // DIAG(F("WifiTransport destroyed"));
}