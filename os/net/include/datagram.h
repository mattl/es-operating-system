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

#ifndef DATAGRAM_H_INCLUDED
#define DATAGRAM_H_INCLUDED

#include <es/clsid.h>
#include <es/endian.h>
#include <es/ring.h>
#include <es/synchronized.h>
#include <es/base/IMonitor.h>
#include "inet.h"
#include "socket.h"

class DatagramReceiver :
    public SocketReceiver
{
    IMonitor*   monitor;
    u8          recvBuf[128 * 1024];
    Ring        recvRing;
    u8          sendBuf[128 * 1024];
    Ring        sendRing;
    Conduit*    conduit;

    struct RingHdr
    {
        long    len;

        RingHdr(long len = 0) :
            len(len)
        {
        }
    };

    Socket* getSocket()
    {
        Conduit* adapter = conduit->getB();
        if (!adapter)
        {
            return 0;
        }
        return dynamic_cast<Socket*>(adapter->getReceiver());
    }

public:
    DatagramReceiver(Conduit* conduit = 0) :
        monitor(0),
        recvRing(recvBuf, sizeof recvBuf),
        sendRing(sendBuf, sizeof sendBuf),
        conduit(conduit)
    {
        esCreateInstance(CLSID_Monitor,
                         IID_IMonitor,
                         reinterpret_cast<void**>(&monitor));
    }

    ~DatagramReceiver()
    {
        if (monitor)
        {
            monitor->release();
        }
    }

    bool input(InetMessenger* m);
    bool output(InetMessenger* m);
    bool error(InetMessenger* m);

    bool read(SocketMessenger* m);
    bool write(SocketMessenger* m);
    bool close(SocketMessenger* m);

    DatagramReceiver* clone(Conduit* conduit, void* key)
    {
        return new DatagramReceiver(conduit);
    }
};

#endif  // DATAGRAM_H_INCLUDED
