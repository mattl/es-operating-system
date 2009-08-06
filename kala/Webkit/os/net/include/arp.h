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

#ifndef ARP_H_INCLUDED
#define ARP_H_INCLUDED

#include <es.h>
#include <es/endian.h>
#include <es/timeSpan.h>
#include <es/net/arp.h>
#include "socket.h"
#include "inet4address.h"

class ARPReceiver : public InetReceiver
{
    InFamily*   inFamily;

public:
    ARPReceiver(InFamily* inFamily) :
        inFamily(inFamily)
    {
    }

    bool input(InetMessenger* m, Conduit* c);
};

class ARPFamily : public AddressFamily
{
    InFamily*                   inFamily;

    // Scope demultiplexer
    InetScopeAccessor           scopeAccessor;
    ConduitFactory              scopeFactory;
    Mux                         scopeMux;

    // ARP protocol
    ARPReceiver                 arpReceiver;
    Protocol                    arpProtocol;
    InetLocalAddressAccessor    arpAccessor;
    Adapter                     arpAdapter;     // prototype
    ConduitFactory              arpFactory;
    Mux                         arpMux;

public:
    ARPFamily(InFamily* inFamily);

    int getAddressFamily()
    {
        return AF_ARP;
    }

    Conduit* getProtocol(Socket* socket)
    {
        return &arpProtocol;
    }

    void addInterface(NetworkInterface* interface)
    {
        Conduit* c = interface->addAddressFamily(this, &scopeMux);
        if (c)
        {
            scopeMux.addB(reinterpret_cast<void*>(interface->getScopeID()), c);
        }
    }

    void addAddress(Inet4Address* address)
    {
        ASSERT(address);
        if (address)
        {
            InetMessenger m;
            m.setLocal(address);
            Installer installer(&m);
            arpMux.accept(&installer, &arpProtocol);
        }
    }

    void removeAddress(Inet4Address* address)
    {
        ASSERT(address);
        if (address)
        {
            Adapter* adapter = dynamic_cast<Adapter*>(address->getAdapter());
            if (adapter)
            {
                InetMessenger m;
                m.setLocal(address);
                Uninstaller uninstaller(&m);
                adapter->accept(&uninstaller);
            }
        }
    }
};

#endif  // ARP_H_INCLUDED
