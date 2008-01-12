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

#ifndef ADDRESS_H_INCLUDED
#define ADDRESS_H_INCLUDED

#include <string.h>
#include <es/collection.h>
#include <es/list.h>
#include <es/net/IInternetAddress.h>
#include "conduit.h"

using namespace es;

class Socket;

class Address : public IInternetAddress
{
    u8                      mac[6];
    Collection<Socket*>     sockets;
    Collection<Messenger*>  packets;

public:
    Address()
    {
        memset(mac, 0, sizeof mac);
    }

    virtual ~Address() {}

    void getMacAddress(u8 mac[6]) const
    {
        memmove(mac, this->mac, sizeof this->mac);
    }

    void setMacAddress(u8 mac[6])
    {
        memmove(this->mac, mac, sizeof this->mac);
    }

    void addSocket(Socket* socket)
    {
        sockets.addLast(socket);
        start();
    }

    void removeSocket(Socket* socket)
    {
        sockets.remove(socket);
        if (sockets.isEmpty())
        {
            stop();
        }
    }

    virtual s32 sumUp() = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual Address* getNextHop() = 0;

    void hold(Messenger* m)
    {
        m->addRef();
        packets.addLast(m);
    }

    Messenger* retrieve()
    {
        if (packets.isEmpty())
        {
            return 0;
        }
        return packets.removeFirst();
    }

    friend class UDPReceiver;
};

template <class Address>
class AddressSet
{
    // A node should retain at least two entries in the Default Router List [RFC 2461]
    static const int MaxAddresses = 2;

    Address* list[MaxAddresses];

public:
    AddressSet()
    {
        for (int i = 0; i < MaxAddresses; ++i)
        {
            list[i] = 0;
        }
    }
    ~AddressSet()
    {
        for (int i = 0; i < MaxAddresses; ++i)
        {
            if (list[i])
            {
                list[i]->release();
            }
        }
    }
    void addAddress(Address* address)
    {
        if (address)
        {
            address->addRef();
            for (int i = 0; i < MaxAddresses; ++i)
            {
                if (!list[i])
                {
                    list[i] = address;
                    return;
                }
            }
            shuffle();
            list[MaxAddresses - 1]->release();
            list[MaxAddresses - 1] = address;
        }
    }
    void removeAddress(Address* address)
    {
        for (int i = 0; i < MaxAddresses; ++i)
        {
            if (list[i] == address)
            {
                list[i] = 0;
                address->release();
                return;
            }
        }
    }
    Address* getAddress()
    {
        Address* address = list[0];
        if (address)
        {
            address->addRef();
        }
        return address;
    }
    void shuffle()
    {
        Address* tmp = list[0];
        int i;
        for (i = 0; i < MaxAddresses - 1; ++i)
        {
            if (list[i + 1])
            {
                list[i] = list[i + 1];
            }
            else
            {
                list[i] = tmp;
                return;
            }
        }
        ASSERT(i == MaxAddresses - 1);
        list[i] = tmp;
    }
};

#endif  // ADDRESS_H_INCLUDED
