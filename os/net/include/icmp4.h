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

#ifndef ICMP4_H_INCLUDED
#define ICMP4_H_INCLUDED

#include <es/clsid.h>
#include <es/endian.h>
#include <es/synchronized.h>
#include <es/base/IMonitor.h>
#include <es/net/icmp.h>
#include "inet4address.h"

class ICMPAccessor : public Accessor
{
public:
    /** @return the type field of IPHdr as the key.
     */
    void* getKey(Messenger* m)
    {
        ASSERT(m);

        ICMPHdr* icmphdr = static_cast<ICMPHdr*>(m->fix(sizeof(ICMPHdr)));
        return reinterpret_cast<void*>(icmphdr->type);
    }
};

class ICMPReceiver : public InetReceiver
{
    s16 checksum(InetMessenger* m);
public:
    bool input(InetMessenger* m);
    bool output(InetMessenger* m);
};

class ICMPEchoRequestReceiver : public InetReceiver
{
    Conduit*        adapter;
    Inet4Address*   addr;

public:
    ICMPEchoRequestReceiver(Conduit* adapter, Inet4Address* addr) :
        adapter(adapter),
        addr(addr)
    {
        if (addr)
        {
            addr->addRef();
        }
    }

    ~ICMPEchoRequestReceiver()
    {
        if (addr)
        {
            addr->release();
        }
    }

    bool input(InetMessenger* m);

    Inet4Address* getAddress()
    {
        if (addr)
        {
            addr->addRef();
        }
        return addr;
    }

    ICMPEchoRequestReceiver* clone(Conduit* conduit, void* key)
    {
        return new ICMPEchoRequestReceiver(conduit, static_cast<Inet4Address*>(key));
    }
};

class ICMPEchoReplyReceiver : public InetReceiver
{
    IMonitor*       monitor;
    Adapter*        adapter;
    Inet4Address*   addr;
    bool            replied;
public:
    ICMPEchoReplyReceiver(Adapter* adapter, Inet4Address* addr) :
        adapter(adapter),
        addr(addr),
        replied(false)
    {
        esCreateInstance(CLSID_Monitor,
                         IID_IMonitor,
                         reinterpret_cast<void**>(&monitor));
    }
    ~ICMPEchoReplyReceiver()
    {
        if (monitor)
        {
            monitor->release();
        }
    }

    bool input(InetMessenger* m);

    bool wait(s64 timeout)
    {
        Synchronized<IMonitor*> method(monitor);
        monitor->wait(timeout);
    }

    void notify()
    {
        Synchronized<IMonitor*> method(monitor);
        replied = true;
        monitor->notifyAll();
    }

    bool isReplied()
    {
        return replied;
    }
};

#endif  // ICMP4_H_INCLUDED
