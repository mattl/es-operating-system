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

#ifndef ADDRESS_H_INCLUDED
#define ADDRESS_H_INCLUDED

#include <string.h>
#include <es/collection.h>
#include <es/list.h>
#include <es/net/IInternetAddress.h>
#include "conduit.h"

class Socket;

class Address : public es::InternetAddress
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
