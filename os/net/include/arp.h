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

    bool input(InetMessenger* m);
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

    void addInterface(Interface* interface)
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
            InetMessenger m;
            m.setLocal(address);
            Uninstaller uninstaller(&m);
            arpMux.accept(&uninstaller, &arpProtocol);

            Adapter* adapter = dynamic_cast<Adapter*>(uninstaller.getConduit());
            if (adapter)
            {
                delete adapter;
                address->release();
            }
        }
    }
};

#endif  // ARP_H_INCLUDED
