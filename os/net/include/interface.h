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

#ifndef INTERFACE_H_INCLUDED
#define INTERFACE_H_INCLUDED

#include <es/base/IStream.h>
#include <es/base/IThread.h>
#include "inet.h"
#include "interface.h"
#include "address.h"

class AddressFamily;

IThread* esCreateThread(void* (*start)(void* param), void* param);

/** This class represents a network interface like Ethernet interface,
 *  loopback interface, etc.
 */
class Interface
{
    IThread*        thread;
    u8              chunk[1500];

    u8              mac[6];     // MAC address

    IStream*        stream;     // Raw stream
    Accessor*       accessor;
    Receiver*       receiver;

    ConduitFactory  factory;
    Adapter         adapter;
    int             scopeID;

    void* vent()
    {
        while (stream)
        {
            int len = stream->read(chunk, sizeof(chunk));
            if (0 < len)
            {
                esReport("# input\n");
                esDump(chunk, len);
                InetMessenger m(&InetReceiver::input, chunk, len);
                m.setScopeID(scopeID);
                Transporter v(&m);
                adapter.accept(&v);
            }
        }
        return 0;
    }

    static void* run(void* param)
    {
        Interface* interface = static_cast<Interface*>(param);
        return interface->vent();
    }

protected:
    Mux             mux;

public:
    Interface(IStream* stream, Accessor* accessor, Receiver* receiver) :
        stream(stream),
        accessor(accessor),
        receiver(receiver),
        mux(accessor, &factory),
        scopeID(0),
        thread(0)
    {
        memset(mac, 0, sizeof mac);

        adapter.setReceiver(receiver);
        Conduit::connectAA(&adapter, &mux);

        if (stream)
        {
            stream->addRef();
        }
    }
    ~Interface()
    {
        if (stream)
        {
            stream->release();
            stream = 0;
        }
    }

    IStream* getStream()
    {
        if (stream)
        {
            stream->addRef();
        }
        return stream;
    }

    Adapter* getAdapter()
    {
        return &adapter;
    }

    int getScopeID()
    {
        return scopeID;
    }
    void setScopeID(int id)
    {
        scopeID = id;
    }

    void getMacAddress(u8 mac[6]) const
    {
        memmove(mac, this->mac, sizeof this->mac);
    }

    void setMacAddress(u8 mac[6])
    {
        memmove(this->mac, mac, sizeof this->mac);
    }

    /** Run a thread that reads from the stream and creates an input
     * messenger to be accepted by the interface adapter.
     */
    void start()
    {
        thread = esCreateThread(run, this);
        thread->start();
    }

    virtual Conduit* addAddressFamily(AddressFamily* af, Conduit* c) = 0;

    friend class Socket;
};

#endif  // INTERFACE_H_INCLUDED
