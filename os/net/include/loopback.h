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
    Handle<IStream> stream;

public:
    LoopbackReceiver(INetworkInterface* loopbackInterface) :
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

class LoopbackInterface : public Interface
{
    static LoopbackAccessor loopbackAccessor;

    LoopbackReceiver        loopbackReceiver;

public:
    LoopbackInterface(INetworkInterface* loopbackInterface) :
        loopbackReceiver(loopbackInterface),
        Interface(loopbackInterface, &loopbackAccessor, &loopbackReceiver)
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
