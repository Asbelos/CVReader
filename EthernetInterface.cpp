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
 * 
 *  Ethernet Interface added by Gregor Baues
 */

#include <EthernetInterface.h>
#include "DIAG.h"
#include "StringFormatter.h"

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

EthernetServer EthernetInterface::server = EthernetServer(LISTEN_PORT); // Ethernet Server listening on default port LISTEN_PORT
IPAddress EthernetInterface::ip = IPAddress(IP_ADDRESS);                // init with fixed IP address needed to get to the server
IPAddress EthernetInterface::dnsip;                                     // DNS IP address
uint16_t EthernetInterface::port;                                       // Port Number
byte EthernetInterface::mac[6] = MAC_ADDRESS;                           // MAC Address as defined in .h
bool EthernetInterface::connected = false;                              // Connection status
protocolCallback EthernetInterface::protocolHandler;                    // Function pointer for the protocol handler either for UDP or TCP
EthernetUDP Udp;                                                        // An EthernetUDP instance to let us send and receive packets over UDP
DCCEXParser ethParser;                                                  // Parser and execute incomming DCC++ EX commands
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];                              // buffer to hold incoming UDP packet,
uint8_t buffer[MAX_ETH_BUFFER];                                         // buffer provided to the streamer to be filled with the reply (used by TCP also for the recv)
MemStream streamer(buffer, MAX_ETH_BUFFER, MAX_ETH_BUFFER, true);       // streamer who writes the results to the buffer
EthernetClient clients[MAX_SOCK_NUM];                                   // accept up to MAX_SOCK_NUM client connections at the same time; This depends on the chipset used on the Shield

// Support Functions
/**
 * @brief Aquire IP Address from DHCP; if that fails try a statically configured address
 * 
 * @return true 
 * @return false 
 */
static bool setupConnection()
{

    DIAG(F("\nInitialize Ethernet with DHCP:"));
    if (Ethernet.begin(EthernetInterface::mac) == 0)
    {
        DIAG(F("\nFailed to configure Ethernet using DHCP ... Trying with fixed IP"));
        Ethernet.begin(EthernetInterface::mac, EthernetInterface::ip); // default ip address

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

    EthernetInterface::ip = Ethernet.localIP(); // reassign the obtained ip address

    DIAG(F("\nLocal IP address:      [%d.%d.%d.%d]"), EthernetInterface::ip[0], EthernetInterface::ip[1], EthernetInterface::ip[2], EthernetInterface::ip[3]);
    DIAG(F("\nListening on port:     [%d]"), EthernetInterface::port);
    EthernetInterface::dnsip = Ethernet.dnsServerIP();
    DIAG(F("\nDNS server IP address: [%d.%d.%d.%d] "), EthernetInterface::ip[0], EthernetInterface::ip[1], EthernetInterface::ip[2], EthernetInterface::ip[3]);
    return true;
}

/**
 * @brief Handles command requests recieved via UDP. UDP is a connection less, unreliable protocol as it doesn't maintain state but fast.
 * 
 */
static void udpHandler()
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
        Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);

        DIAG(F("Command:                [%s]\n"), packetBuffer);

        streamer.flush();

        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());

        ethParser.parse(&streamer, (byte *)packetBuffer, true); // set to true so it is sync cf. WifiInterface

        if (streamer.available() == 0)
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

        memset(packetBuffer, 0, UDP_TX_PACKET_MAX_SIZE); // reset PacktBuffer
        return;
    }
}

/**
 * @brief Handles command requests recieved via TCP. Supports up to the max# of simultaneous requests which is 8. The connection gets closed as soon as we finished processing
 * 
 */
static void tcpHandler()
{
    // get client from the server
    EthernetClient client = EthernetInterface::getServer().accept();

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
            buffer[count] = '\0'; // terminate the string properly
            DIAG(F("\nReceived packet of size:[%d]\n"), count);
            DIAG(F("From Client #:          [%d]\n"), i);
            DIAG(F("Command:                [%s]\n"), buffer);

            // as we use buffer for recv and send we have to reset the write position
            streamer.setBufferContentPosition(0, 0);

            ethParser.parse(&streamer, buffer, true); // set to true to that the execution in DCC is sync

            if (streamer.available() == 0)
            {
                DIAG(F("No response\n"));
            }
            else
            {
                buffer[streamer.available()] = '\0'; // mark end of buffer, so it can be used as a string later
                DIAG(F("Response: %s\n"), (char *)buffer);
                if (clients[i].connected())
                {
                    clients[i].write(buffer, streamer.available());
                }
            }
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

// Class Functions
/**
 * @brief Setup Ethernet Connection
 * 
 * @param pt         Protocol used
 * @param localPort  Port number for the connection
 */
void EthernetInterface::setup(protocolType pt, uint16_t localPort)
{
    DIAG(F("\n++++++ Ethernet Setup In Progress ++++++++\n"));
    EthernetInterface::port = localPort;
    if (setupConnection())
    {
        DIAG(F("\nProtocol:              [%s]\n"), pt ? "UDP" : "TCP");
        switch (pt)
        {
        case UDP:
        {
            if (Udp.begin(localPort))
            {
                EthernetInterface::connected = true;
                EthernetInterface::protocolHandler = udpHandler;
            }
            else
            {
                DIAG(F("\nUDP client failed to start"));
                EthernetInterface::connected = false;
            }
            break;
        };
        case TCP:
        {
            Ethernet.begin(mac, ip);
            EthernetServer server(localPort);
            EthernetInterface::setServer(server);
            server.begin();
            EthernetInterface::connected = true;
            EthernetInterface::protocolHandler = tcpHandler;
            break;
        };
        default:
        {
            DIAG(F("Unkown Ethernet protocol; Setup failed"));
            EthernetInterface::connected = false;
            return;
        }
        }
    }
    else
    {
        EthernetInterface::connected = false;
    };
    DIAG(F("\n++++++ Ethernet Setup %S ++++++++\n"), EthernetInterface::connected ? F("OK") : F("FAILED"));
};

/**
 * @brief Setup Ethernet on default port and user choosen protocol
 * 
 * @param pt Protocol UDP or TCP
 */
void EthernetInterface::setup(protocolType pt)
{
    setup(pt, LISTEN_PORT);
};

/**
 * @brief Ethernet setup  with defaults TCP / Listen Port
 * 
 */
void EthernetInterface::setup()
{
    setup(TCP, LISTEN_PORT);
}

/**
 * @brief Main loop for the EthernetInterface
 * 
 */
void EthernetInterface::loop()
{
    switch (Ethernet.maintain())
    {
    case 1:
        //renewed fail
        DIAG(F("\nError: renewed fail"));
        break;

    case 2:
        //renewed success
        DIAG(F("\nRenewed success: "));
        EthernetInterface::ip = Ethernet.localIP(); // reassign the obtained ip address
        DIAG(F("\nLocal IP address:      [%d.%d.%d.%d]"), EthernetInterface::ip[0], EthernetInterface::ip[1], EthernetInterface::ip[2], EthernetInterface::ip[3]);
        break;

    case 3:
        //rebind fail
        DIAG(F("Error: rebind fail"));
        break;

    case 4:
        //rebind success
        DIAG(F("Rebind success"));
        EthernetInterface::ip = Ethernet.localIP(); // reassign the obtained ip address
        DIAG(F("\nLocal IP address:      [%d.%d.%d.%d]"), EthernetInterface::ip[0], EthernetInterface::ip[1], EthernetInterface::ip[2], EthernetInterface::ip[3]);
        break;

    default:
        //nothing happened
        break;
    }
    EthernetInterface::protocolHandler();
}