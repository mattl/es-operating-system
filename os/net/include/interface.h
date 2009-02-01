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

#ifndef INTERFACE_H_INCLUDED
#define INTERFACE_H_INCLUDED

#include <es/handle.h>
#include <es/base/IStream.h>
#include <es/base/IThread.h>
#include <es/device/INetworkInterface.h>
#include "inet.h"
#include "address.h"

class AddressFamily;

es::Thread* esCreateThread(void* (*start)(void* param), void* param);

/** This class represents a network interface like Ethernet interface,
 *  loopback interface, etc.
 */
class NetworkInterface
{
    static const int MRU = 1518;

    Handle<es::NetworkInterface>   networkInterface;

    es::Thread*     thread;

    u8              mac[6];             // MAC address

    Accessor*       accessor;
    Receiver*       receiver;

    ConduitFactory  factory;
    Adapter         adapter;
    int             scopeID;

    void* vent()
    {
        Handle<InetMessenger> m = new InetMessenger(&InetReceiver::input, MRU);
        Handle<es::Stream> stream = networkInterface;
        for (;;)
        {
            int len = stream->read(m->fix(MRU), MRU);
            if (0 < len)
            {
#ifdef VERBOSE
                esReport("# input\n");
                esDump(m->fix(len), len);
#endif
                m->setSize(len);
                m->setScopeID(scopeID);
                Transporter v(m);
                adapter.accept(&v);
                m->setSize(MRU);    // Restore the size
                m->setPosition(0);

                m->setLocal(0);
                m->setRemote(0);
            }
        }
        return 0;
    }

    static void* run(void* param)
    {
        NetworkInterface* interface = static_cast<NetworkInterface*>(param);
        return interface->vent();
    }

protected:
    Mux             mux;

public:
    NetworkInterface(es::NetworkInterface* networkInterface, Accessor* accessor, Receiver* receiver) :
        networkInterface(networkInterface, true),
        thread(0),
        accessor(accessor),
        receiver(receiver),
        mux(accessor, &factory),
        scopeID(0)
    {
        memset(mac, 0, sizeof mac);

        adapter.setReceiver(receiver);
        Conduit::connectAA(&adapter, &mux);
    }

    es::NetworkInterface* getNetworkInterface()
    {
        return networkInterface;    // XXX Check reference count
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

    void addMulticastAddress(u8 mac[6])
    {
        networkInterface->addMulticastAddress(mac);
    }

    void removeMulticastAddress(u8 mac[6])
    {
        networkInterface->removeMulticastAddress(mac);
    }

    /** Run a thread that reads from the stream and creates an input
     * messenger to be accepted by the interface adapter.
     */
    void start()
    {
        thread = esCreateThread(run, this);
        thread->setPriority(es::Thread::Highest);
        thread->start();
    }

    virtual Conduit* addAddressFamily(AddressFamily* af, Conduit* c) = 0;

    friend class Socket;
};

#endif  // INTERFACE_H_INCLUDED
