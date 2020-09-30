#include <Arduino.h>
#include <Ethernet.h>

#include "DIAG.h"
#include "NetworkInterface.h"
#include "EthernetTransport.h"

EthernetServer EthernetTransport::server = EthernetServer(LISTEN_PORT);

void EthernetTransport::udpHandler()
{
    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
        DIAG(F("\nReceived packet of size:[%d]\n"), packetSize);
        IPAddress remote = Udp.remoteIP();
        DIAG(F("From:                   [%d.%d.%d.%d:"), remote[0], remote[1], remote[2], remote[3]);
        char portBuffer[6];
        DIAG(F("%d]\n"),Udp.remotePort()); 

        // read the packet into packetBufffer
        packetBuffer[Udp.read(packetBuffer, MAX_ETH_BUFFER-1)]=0;
        DIAG(F("Command:                [%s]\n"), packetBuffer);

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

        memset(packetBuffer, 0, MAX_ETH_BUFFER); // reset PacktBuffer
        return;
    }
}

void EthernetTransport::tcpHandler()
{
    // get client from the server
    EthernetClient client = server.accept();
    
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
            int count = clients[i].read(buffer, MAX_ETH_BUFFER-1);
            buffer[count]=0;
            IPAddress remote = client.remoteIP();
            buffer[count] = '\0'; // terminate the string properly
            DIAG(F("\nReceived packet of size:[%d] from [%d.%d.%d.%d]\n"), count, remote[0], remote[1], remote[2], remote[3]);
            DIAG(F("Client #:               [%d]\n"), i);
            DIAG(F("Command:                [%e]\n"), buffer);

            parse(&(clients[i]), buffer, true);
            
        }
        // stop any clients which disconnect
        for (byte i = 0; i < MAX_SOCK_NUM; i++)
        {
            if (clients[i] && !clients[i].connected())
            {
                DIAG(F("Disconnect client #%d \n"), i);
                clients[i].stop();
                clients[i]=0;
            }
        }
    }
}

uint8_t EthernetTransport::setup()
{
    p = (protocolType)protocol;
    
    DIAG(F("\nInitialize Ethernet with DHCP"));
    if (Ethernet.begin(mac) == 0)
    {
        DIAG(F("\nFailed to configure Ethernet using DHCP ... Trying with fixed IP"));
        Ethernet.begin(mac, IPAddress(IP_ADDRESS)); // default ip address

        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
            DIAG(F("\nEthernet shield was not found. Sorry, can't run without hardware. :("));
            return false;
        };
        if (Ethernet.linkStatus() == LinkOFF)
        {
            DIAG(F("\nEthernet cable is not connected."));
            return false;
        }
    }

    // set the obtained ip address
    ip = Ethernet.localIP();

    // Setup the protocol handler
    DIAG(F("\nNetwork Protocol:      [%s]"), p ? "UDP" : "TCP");
    switch (p)
    {
    case UDP:
    {
        if (Udp.begin(port))
        {
            connected = true;
            ip = Ethernet.localIP();
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
        server = EthernetServer(port);
        server.begin();
        connected = true;
        
        break;
    };
    default:
    {
        DIAG(F("\nUnkown Ethernet protocol; Setup failed"));
        connected = false;
        break;
    }
    }

    if (connected)
    {
        DIAG(F("\nLocal IP address:      [%d.%d.%d.%d]"), ip[0], ip[1], ip[2], ip[3]);
        DIAG(F("\nListening on port:     [%d]"), port);
        dnsip = Ethernet.dnsServerIP();
        DIAG(F("\nDNS server IP address: [%d.%d.%d.%d] "), dnsip[0], dnsip[1], dnsip[2], dnsip[3]);
        return 1;
    }

    // something went wrong
    return 0;
}

void EthernetTransport::loop()
{
    // DIAG(F("Loop .. "));
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

EthernetTransport::EthernetTransport()
{
     // DIAG(F("EthernetTransport created "));
}

EthernetTransport::~EthernetTransport()
{
    // DIAG(F("EthernetTransport destroyed"));
}
