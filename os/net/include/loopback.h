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

#ifndef LOOPBACK_H_INCLUDED
#define LOOPBACK_H_INCLUDED

#include <es.h>
#include <es/endian.h>
#include <es/ref.h>
#include <es/ring.h>
#include <es/base/IStream.h>
#include "inet.h"
#include "interface.h"

class LoopbackAccessor : public Accessor
{
public:
    void* getKey(Messenger* m)
    {
        ASSERT(m);
        int af;

        m->read(&af, sizeof(int), m->getPosition());
        m->movePosition(sizeof(int));
        return reinterpret_cast<void*>(af);
    }
};

class LoopbackReceiver :
    public InetReceiver
{
    Handle<es::Stream> stream;

public:
    LoopbackReceiver(es::NetworkInterface* loopbackInterface) :
        stream(loopbackInterface, true)
    {
        ASSERT(stream);
    }

    bool output(InetMessenger* m, Conduit* c)
    {
        int af = m->getType();
        m->movePosition(-sizeof(int));
        m->write(&af, sizeof(int), m->getPosition());

        long len = m->getLength();
        void* packet = m->fix(len);
#ifdef VERBOSE
        esReport("# output\n");
        esDump(packet, len);
#endif
        stream->write(packet, len);
        return true;
    }
};

class LoopbackInterface : public NetworkInterface
{
    static LoopbackAccessor loopbackAccessor;

    LoopbackReceiver        loopbackReceiver;

public:
    LoopbackInterface(es::NetworkInterface* loopbackInterface) :
        loopbackReceiver(loopbackInterface),
        NetworkInterface(loopbackInterface, &loopbackAccessor, &loopbackReceiver)
    {
    }
    ~LoopbackInterface()
    {
    }

    Conduit* addAddressFamily(AddressFamily* af, Conduit* c)
    {
        int pf = af->getAddressFamily();
        switch (pf)
        {
        case AF_INET:
        case AF_INET6:
            mux.addB(reinterpret_cast<void*>(pf), c);
            return &mux;
        default:
            return 0;
            break;
        }
    }
};

#endif  // LOOPBACK_H_INCLUDED
