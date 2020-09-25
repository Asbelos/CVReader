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

#ifndef EthernetInterface_h
#define EthernetInterface_h

#include "DCCEXParser.h"
#include "MemStream.h"
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <Ethernet.h>

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
#define MAC_ADDRESS { 0x52, 0xB8, 0x8A, 0x8E, 0xCE, 0x21 }      // MAC address of your networking card found on the sticker on your card or take one from above
#define IP_ADDRESS 10, 0, 0, 101                                // Just in case we don't get an adress from DHCP try a static one; make sure 
                                                                // this one is not used elsewhere and corresponds to your network layout
#define LISTEN_PORT 3366                                        // default listen port for the server 
#define MAX_ETH_BUFFER 250

typedef void (*HTTP_CALLBACK)(Print * stream, byte * cmd);

enum protocolType {
    TCP,
    UDP
};

typedef void (*protocolCallback)();

class EthernetInterface {

private:
    static EthernetServer server;

 public:
    static DCCEXParser  ethParser;
    static bool         connected;
    static byte         mac[6];
    static IPAddress    ip;
    static uint16_t     port;
    static IPAddress    dnsip;

    static void setup(protocolType pt, uint16_t lp);        // specific port nummber
    static void setup(protocolType pt);                     // uses default port number
    static void setup();                                    // all defaults (protocol/port)

    static protocolCallback protocolHandler;

    static void loop();

    static EthernetServer getServer() {
        return server;
    };
    static void setServer(EthernetServer s) {
        server = s;
    };
};

#endif
