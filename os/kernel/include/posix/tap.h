/*
 * Copyright (c) 2006
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

#include <es.h>
#include <es/device/IEthernet.h>
#include <es/base/IStream.h>
#include <es/ref.h>
#include "posix/core.h"

class Tap : public IEthernet, public IStream
{
    IMonitor* monitor;
    Ref ref;
    int fd;
    unsigned char mac[6];

    char ifName[256]; // name of this tap interface.
    char bridge[256]; // name of the bridge interface.
    char script[256]; // startup script.

    int setup(void);
    int getBridgeMacAddress(void);

public:
    Tap(const char* ifName, const char* bridge, const char* script);
    ~Tap();

    // IEthernet
    int start();
    int stop();

    int probe()
    {
        return 0;
    }

    bool getPromiscuousMode()
    {
        return false;
    }

    void setPromiscuousMode(bool on)
    {
    }

    int addMulticastAddress(unsigned char macaddr[6])
    {
        return -1;
    }

    int removeMulticastAddress(unsigned char macaddr[6])
    {
        return -1;
    }

    void getMacAddress(unsigned char macaddr[6]);

    bool getLinkState(void)
    {
        return true;
    }

    int getMode(void)
    {
        return MODE_AUTO;
    }

    void getStatistics(InterfaceStatistics* statistics)
    {
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

    // ICallback
    int invoke(int irq);

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);
};

#endif // NINTENDO_ES_KERNEL_POSIX_TAP_H_INCLUDED
