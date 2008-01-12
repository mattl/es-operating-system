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

#include <errno.h>
#include <string.h>
#include <es/clsid.h>
#include <es/exception.h>
#include <es/synchronized.h>
#include "loopback.h"

/*
 *  data format
 *  in the ring buffer
 *
 *  :              :
 *  |              |
 *  +--------------+
 *  |  size (int)  |
 *  + - -  - - - - +
 *  |              |
 *  |  data        |
 *  : (size bytes) :
 *  :              :
 *  :              :
 *  |              |
 *  |              |
 *  |              |
 *  +--------------+
 *  |              |
 *  :              :
 */

//
// IStream
//
long long Loopback::
getPosition()
{
    return 0;
}

void Loopback::
setPosition(long long pos)
{
}

// Gets the number of filled bytes in this ring buffer.
long long Loopback::
getSize()
{
    Monitor::Synchronized method(monitor);
    return ring.getUsed();
}

void Loopback::
setSize(long long size)
{
}

int Loopback::
read(void* dst, int count)
{
    Monitor::Synchronized method(monitor);

    int size;
    int ret;
    while ((ret = ring.peek(&size, sizeof size)) == 0)
    {
        monitor.wait();
    }

    if (ret < 0 || count < ret)
    {
        return -1;
    }

    ring.read(&size, sizeof size);
    ret = ring.read(dst, size);

    monitor.notifyAll();
    return ret;
}

int Loopback::
read(void* dst, int count, long long offset)
{
    return -1;
}

int Loopback::
write(const void* src, int count)
{
    Monitor::Synchronized method(monitor);

    while (ringSize - ring.getUsed() < sizeof(int) + count)
    {
        // no enough space in the ring buffer.
        monitor.wait();
    }

    ring.write(&count, sizeof(int));
    int ret = ring.write(src, count);

    monitor.notifyAll();
    return ret;
}

int Loopback::
write(const void* src, int count, long long offset)
{
    return -1;
}

void Loopback::flush()
{
}

//
// IInterface
//

bool Loopback::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IStream)
    {
        *objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IID_INetworkInterface)
    {
        *objectPtr = static_cast<INetworkInterface*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IStream*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Loopback::
addRef(void)
{
    return ref.addRef();
}

unsigned int Loopback::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
