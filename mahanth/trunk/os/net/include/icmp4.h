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

#ifndef ICMP4_H_INCLUDED
#define ICMP4_H_INCLUDED

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
    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
};

class ICMPEchoRequestReceiver : public InetReceiver
{
    Inet4Address*   addr;

public:
    ICMPEchoRequestReceiver(Inet4Address* addr) :
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

    bool input(InetMessenger* m, Conduit* c);

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
        return new ICMPEchoRequestReceiver(static_cast<Inet4Address*>(key));
    }

    unsigned int release()
    {
        delete this;
        return 0;
    }
};

class ICMPEchoReplyReceiver : public InetReceiver
{
    es::Monitor*       monitor;
    Adapter*        adapter;
    Inet4Address*   addr;
    bool            replied;
public:
    ICMPEchoReplyReceiver(Adapter* adapter, Inet4Address* addr) :
        adapter(adapter),
        addr(addr),
        replied(false)
    {
        monitor = es::Monitor::createInstance();
    }
    ~ICMPEchoReplyReceiver()
    {
        if (monitor)
        {
            monitor->release();
        }
    }

    bool input(InetMessenger* m, Conduit* c);

    bool wait(s64 timeout)
    {
        Synchronized<es::Monitor*> method(monitor);
        monitor->wait(timeout);
    }

    void notify()
    {
        Synchronized<es::Monitor*> method(monitor);
        replied = true;
        monitor->notifyAll();
    }

    bool isReplied()
    {
        return replied;
    }
};

class ICMPUnreachReceiver : public InetReceiver
{
public:
    ICMPUnreachReceiver()
    {
    }

    ~ICMPUnreachReceiver()
    {
    }

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
};

class ICMPSourceQuenchReceiver : public InetReceiver
{
public:
    ICMPSourceQuenchReceiver()
    {
    }

    ~ICMPSourceQuenchReceiver()
    {
    }

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
};

class ICMPTimeExceededReceiver : public InetReceiver
{
public:
    ICMPTimeExceededReceiver()
    {
    }

    ~ICMPTimeExceededReceiver()
    {
    }

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
};

class ICMPParamProbReceiver : public InetReceiver
{
public:
    ICMPParamProbReceiver()
    {
    }

    ~ICMPParamProbReceiver()
    {
    }

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
};

class ICMPRedirectReceiver : public InetReceiver
{
public:
    ICMPRedirectReceiver()
    {
    }

    ~ICMPRedirectReceiver()
    {
    }

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
};

#endif  // ICMP4_H_INCLUDED
