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

#ifndef NINTENDO_ES_KERNEL_LOOPBACK_H_INCLUDED
#define NINTENDO_ES_KERNEL_LOOPBACK_H_INCLUDED

#include <es.h>
#include <es/ref.h>
#include <es/ring.h>
#include <es/base/IStream.h>
#include <es/device/INetworkInterface.h>
#include "thread.h"

class Loopback : public INetworkInterface, public IStream
{
    Ref     ref;
    Monitor monitor;
    Ring    ring;
    long    ringSize;

public:
    Loopback(void* buf, long size) :
        ring(buf, size), ringSize(size)
    {
    }
    ~Loopback()
    {
    }

    // INetworkInterface
    int addMulticastAddress(const unsigned char mac[6])
    {
        return 0;
    }
    void getMacAddress(unsigned char mac[6])
    {
        memset(mac, 0, 6);
    }
    int getMTU()
    {
        return 1500;
    }
    bool getLinkState()
    {
        return true;
    }
    bool getPromiscuousMode()
    {
        return true;
    }
    void getStatistics(INetworkInterface::Statistics* statistics)
    {
        memset(statistics, 0, sizeof(INetworkInterface::Statistics));
    }
    int getType()
    {
        return INetworkInterface::Loopback;
    }
    int removeMulticastAddress(const unsigned char mac[6])
    {
        return 0;
    }
    void setPromiscuousMode(bool on)
    {
    }
    int start()
    {
    }
    int stop()
    {
    }

    // IStream
    long long getPosition();
    void setPosition(long long pos);
    long long getSize();
    void setSize(long long size);
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef();
    unsigned int release();
};

#endif  // NINTENDO_ES_KERNEL_LOOPBACK_H_INCLUDED
