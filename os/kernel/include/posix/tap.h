/*
 * Copyright (c) 2006, 2007
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#ifndef NINTENDO_ES_KERNEL_POSIX_TAP_H_INCLUDED
#define NINTENDO_ES_KERNEL_POSIX_TAP_H_INCLUDED

#include <sys/ioctl.h>
#include <net/if.h>
#include <es.h>
#include <es/ref.h>
#include <es/base/IStream.h>
#include <es/device/INetworkInterface.h>
#include "posix/core.h"

class Tap : public INetworkInterface, public IStream
{
    IMonitor*  monitor;
    Ref        ref;
    int        sd;
    u8         mac[6];
    char       interfaceName[IFNAMSIZ];
    int        ifindex;
    Statistics statistics;

public:
    Tap(const char* interfaceName);
    ~Tap();

    // INetworkInterface
    int getType()
    {
        return INetworkInterface::Ethernet;
    }

    int start();
    int stop();

    bool isPromiscuousMode();
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

    // IStream
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

    // IInterface
    void* queryInterface(const Guid& riid);
    unsigned int addRef(void);
    unsigned int release(void);
};

#endif // NINTENDO_ES_KERNEL_POSIX_TAP_H_INCLUDED
