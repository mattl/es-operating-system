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

#ifndef INET_H_INCLUDED
#define INET_H_INCLUDED

#include <string.h>
#include <es.h>
#include <es/types.h>
#include <es/list.h>
#include "address.h"
#include "conduit.h"

class InetMessenger;

class InetReceiver : public virtual Receiver
{
public:
    virtual bool input(InetMessenger* m, Conduit* c)
    {
        return true;
    }
    virtual bool output(InetMessenger* m, Conduit* c)
    {
        return true;
    }
    virtual bool error(InetMessenger* m, Conduit* c)
    {
        return true;
    }

    typedef bool (InetReceiver::*Command)(InetMessenger* m, Conduit* c);
};

class InetMessenger : public Messenger
{
protected:
    InetReceiver::Command   op;

private:
    int         scopeID;
    Address*    remoteAddress;
    Address*    localAddress;
    u16         remotePort;
    u16         localPort;
    int         code;
    int         flag;

public:
    static const int Unicast = 1;
    static const int Multicast = 2;
    static const int Broadcast = 3;

    InetMessenger(InetReceiver::Command op = 0,
                  long len = 0, long pos = 0, void* chunk = 0) :
        Messenger(len, pos, chunk),
        op(op),
        scopeID(0),
        remoteAddress(0),
        localAddress(0),
        remotePort(0),
        localPort(0),
        code(0),
        flag(0)
    {
    }
    ~InetMessenger()
    {
        setRemote(0);
        setLocal(0);
    }

    virtual bool apply(Conduit* c)
    {
        if (op)
        {
            InetReceiver* receiver = dynamic_cast<InetReceiver*>(c->getReceiver());
            if (receiver)
            {
                return (receiver->*op)(this, c);
            }
        }
        return Messenger::apply(c);
    }

    int getScopeID() const
    {
        return scopeID;
    }
    void setScopeID(int id)
    {
        scopeID = id;
    }

    void setRemote(Address* addr)
    {
        if (addr)
        {
            addr->addRef();
        }
        if (remoteAddress)
        {
            remoteAddress->release();
        }
        remoteAddress = addr;
    }
    Address* getRemote() const
    {
        if (remoteAddress)
        {
            remoteAddress->addRef();
        }
        return remoteAddress;
    }
    void setLocal(Address* addr)
    {
        if (addr)
        {
            addr->addRef();
        }
        if (localAddress)
        {
            localAddress->release();
        }
        localAddress = addr;
    }
    Address* getLocal() const
    {
        if (localAddress)
        {
            localAddress->addRef();
        }
        return localAddress;
    }

    int getRemotePort()
    {
        return remotePort;
    }
    int getLocalPort()
    {
        return localPort;
    }
    void setRemotePort(int port)
    {
        remotePort = port;
    }
    void setLocalPort(int port)
    {
        localPort = port;
    }

    int getErrorCode() const
    {
        return code;
    }
    void setErrorCode(int code)
    {
        this->code = code;
    }

    int getFlag() const
    {
        return flag;
    }
    void setFlag(int flag)
    {
        this->flag = flag;
    }

    void setCommand(InetReceiver::Command command)
    {
        op = command;
    }

    friend class InetLocalAddressAccessor;
    friend class InetRemoteAddressAccessor;
};

class InetLocalPortAccessor : public Accessor
{
public:
    void* getKey(Messenger* m)
    {
        InetMessenger* im = dynamic_cast<InetMessenger*>(m);
        ASSERT(im);
        return reinterpret_cast<void*>(im->getLocalPort());
    }
};

class InetRemotePortAccessor : public Accessor
{
public:
    void* getKey(Messenger* m)
    {
        InetMessenger* im = dynamic_cast<InetMessenger*>(m);
        ASSERT(im);
        return reinterpret_cast<void*>(im->getRemotePort());
    }
};

class InetLocalAddressAccessor : public Accessor
{
public:
    void* getKey(Messenger* m)
    {
        InetMessenger* im = dynamic_cast<InetMessenger*>(m);
        ASSERT(im);
        return im->localAddress;    // Just need the address as the key
    }
};

class InetRemoteAddressAccessor : public Accessor
{
public:
    void* getKey(Messenger* m)
    {
        InetMessenger* im = dynamic_cast<InetMessenger*>(m);
        ASSERT(im);
        return im->remoteAddress;   // Just need the address as the key
    }
};

class InetScopeAccessor : public Accessor
{
public:
    void* getKey(Messenger* m)
    {
        InetMessenger* im = dynamic_cast<InetMessenger*>(m);
        ASSERT(im);
        return reinterpret_cast<void*>(im->getScopeID());
    }
};

#endif // INET_H_INCLUDED
