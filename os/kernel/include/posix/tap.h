/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NINTENDO_ES_KERNEL_POSIX_TAP_H_INCLUDED
#define NINTENDO_ES_KERNEL_POSIX_TAP_H_INCLUDED

#ifdef __linux__

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <es.h>
#include <es/ref.h>
#include <es/base/IStream.h>
#include <es/device/INetworkInterface.h>
#include "posix/core.h"

class Tap : public es::NetworkInterface, public es::Stream
{
    struct Statistics
    {
        unsigned long long  inOctets;        // The total number of octets received.
        unsigned int        inUcastPkts;     // The number of unicast packets delivered.
        unsigned int        inNUcastPkts;    // The number of non-unicast delivered.
        unsigned int        inDiscards;      // The number of inbound packets discarded.
        unsigned int        inErrors;        // The number of inbound packets that contained errors.
        unsigned int        inUnknownProtos; // The number of inbound packets discarded because of an unknown or unsupported protocol.
        unsigned long long  outOctets;       // The total number of octets transmitted.
        unsigned int        outUcastPkts;    // The total number of packets transmitted to a unicast address.
        unsigned int        outNUcastPkts;   // The total number of packets transmitted to a non-unicast address.
        unsigned int        outDiscards;     // The number of outbound packets discarded.
        unsigned int        outErrors;       // The number of outbound packets that could not be transmitted because of errors.

        unsigned int        outCollisions;   // Collisions on CSMA
    };

    es::Monitor*  monitor;
    Ref        ref;
    int        sd;
    u8         mac[6];
    char       interfaceName[IFNAMSIZ];
    int        ifindex;
    Statistics statistics;

public:
    Tap(const char* interfaceName);
    ~Tap();

    // es::NetworkInterface
    int getType()
    {
        return es::NetworkInterface::Ethernet;
    }

    int start();
    int stop();

    bool getPromiscuousMode();
    void setPromiscuousMode(bool on);

    int addMulticastAddress(const unsigned char macaddr[6]);
    int removeMulticastAddress(const unsigned char macaddr[6]);

    void getMacAddress(unsigned char macaddr[6]);

    bool getLinkState();

    unsigned long long getInOctets()
    {
        return statistics.inOctets;
    }
    unsigned int getInUcastPkts()
    {
        return statistics.inUcastPkts;
    }
    unsigned int getInNUcastPkts()
    {
        return statistics.inNUcastPkts;
    }
    unsigned int getInDiscards()
    {
        return statistics.inDiscards;
    }
    unsigned int getInErrors()
    {
        return statistics.inErrors;
    }
    unsigned int getInUnknownProtos()
    {
        return statistics.inUnknownProtos;
    }
    unsigned long long getOutOctets()
    {
        return statistics.outOctets;
    }
    unsigned int getOutUcastPkts()
    {
        return statistics.outUcastPkts;
    }
    unsigned int getOutNUcastPkts()
    {
        return statistics.outNUcastPkts;
    }
    unsigned int getOutDiscards()
    {
        return statistics.outDiscards;
    }
    unsigned int getOutErrors()
    {
        return statistics.outErrors;
    }
    unsigned int getOutCollisions()
    {
        return statistics.outCollisions;
    }
    int getMTU()
    {
        return 1500;
    }

    // es::Stream
    long long getPosition()
    {
        return 0;
    }

    void setPosition(long long pos)
    {
    }

    long long getSize()
    {
        return 0;
    }

    void setSize(long long size)
    {
    }

    int read(void* dst, int count);
    int read(void* dst, int count, long long offset)
    {
        return -1;
    }

    int write(const void* src, int count);
    int write(const void* src, int count, long long offset)
    {
        return -1;
    }

    void flush()
    {
    }

    // Object
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

#endif  // __linux__

#endif  // NINTENDO_ES_KERNEL_POSIX_TAP_H_INCLUDED
