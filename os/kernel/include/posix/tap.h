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

    void getStatistics(Statistics* statistics);

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
