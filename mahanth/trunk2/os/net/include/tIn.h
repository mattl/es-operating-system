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

#ifndef TIN_H_INCLUDED
#define TIN_H_INCLUDED

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

class TInterface;

class TAccessor : public Accessor
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

class TReceiver : public InetReceiver
{
    TInterface* dix;

public:
    TReceiver(TInterface* dix) :
        dix(dix)
    {
    }

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
};

class TInReceiver : public InetReceiver
{
    TInterface* dix;

public:
    TInReceiver(TInterface* dix) :
        dix(dix)
    {
    }
    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
};

class TARPReceiver : public InetReceiver
{
    TInterface* dix;

public:
    TARPReceiver(TInterface* dix) :
        dix(dix)
    {
    }

    bool output(InetMessenger* m, Conduit* c);
};

class TInterface : public NetworkInterface
{
public:
    Handle<es::NetworkInterface>   networkInterface;
    Handle<es::Stream>             stream;

    TAccessor         dixAccessor;
    TReceiver         dixReceiver;
    TInReceiver       inReceiver;
    TARPReceiver      arpReceiver;

    Protocol            inProtocol;     // DIX_IP
    Protocol            arpProtocol;    // DIX_ARP
    Protocol            in6Protocol;    // DIX_IPv6

    static const u8 macAllHost[6];

public:
    TInterface(es::NetworkInterface* networkInterface);

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

#endif  // TIN_H_INCLUDED
