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

#ifndef DIX_H_INCLUDED
#define DIX_H_INCLUDED

#include <string.h>
#include <es.h>
#include <es/endian.h>
#include <es/handle.h>
#include <es/base/IStream.h>
#include <es/device/INetworkInterface.h>
#include <es/net/dix.h>
#include <es/net/inet4.h>
#include <es/net/inet6.h>
#include "inet4.h"
#include "interface.h"
#include "socket.h"

class DIXInterface;

class DIXAccessor : public Accessor
{
public:
    void* getKey(Messenger* m)
    {
        ASSERT(m);

        DIXHdr* dixhdr = static_cast<DIXHdr*>(m->fix(sizeof(DIXHdr)));
        m->movePosition(sizeof(DIXHdr));
        return reinterpret_cast<void*>(ntohs(dixhdr->type));
    }
};

class DIXReceiver : public InetReceiver
{
    DIXInterface* dix;

public:
    DIXReceiver(DIXInterface* dix) :
        dix(dix)
    {
    }

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
};

class DIXInReceiver : public InetReceiver
{
    DIXInterface* dix;

public:
    DIXInReceiver(DIXInterface* dix) :
        dix(dix)
    {
    }

    bool output(InetMessenger* m, Conduit* c);
};

class DIXARPReceiver : public InetReceiver
{
    DIXInterface* dix;

public:
    DIXARPReceiver(DIXInterface* dix) :
        dix(dix)
    {
    }

    bool output(InetMessenger* m, Conduit* c);
};

class DIXInterface : public Interface
{
    Handle<INetworkInterface>   networkInterface;
    Handle<IStream>             stream;

    DIXAccessor         dixAccessor;
    DIXReceiver         dixReceiver;
    DIXInReceiver       inReceiver;
    DIXARPReceiver      arpReceiver;

    Protocol            inProtocol;     // DIX_IP
    Protocol            arpProtocol;    // DIX_ARP
    Protocol            in6Protocol;    // DIX_IPv6

    static const u8 macAllHost[6];

public:
    DIXInterface(INetworkInterface* networkInterface);

    Conduit* addAddressFamily(AddressFamily* af, Conduit* c)
    {
        switch (af->getAddressFamily())
        {
        case AF_INET:
            // Join all hosts group by default
            networkInterface->addMulticastAddress(macAllHost);
            inProtocol.setB(c);
            return &inProtocol;
            break;
        case AF_ARP:
            arpProtocol.setB(c);
            return &arpProtocol;
            break;
        case AF_INET6:
            in6Protocol.setB(c);
            return &in6Protocol;
            break;
        default:
            break;
        }
        return 0;
    }

    int read(void* dst, int count)
    {
        if (stream)
        {
            return stream->read(dst, count);
        }
        return 0;
    }

    int write(const void* src, int count)
    {
        if (stream)
        {
            return stream->write(src, count);
        }
        return 0;
    }
};

#endif  // DIX_H_INCLUDED
