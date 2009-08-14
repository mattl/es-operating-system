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

class DIXInterface : public NetworkInterface
{
    Handle<es::NetworkInterface>   networkInterface;
    Handle<es::Stream>             stream;

    DIXAccessor         dixAccessor;
    DIXReceiver         dixReceiver;
    DIXInReceiver       inReceiver;
    DIXARPReceiver      arpReceiver;

    Protocol            inProtocol;     // DIX_IP
    Protocol            arpProtocol;    // DIX_ARP
    Protocol            in6Protocol;    // DIX_IPv6

    static const u8 macAllHost[6];

public:
    DIXInterface(es::NetworkInterface* networkInterface);

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
