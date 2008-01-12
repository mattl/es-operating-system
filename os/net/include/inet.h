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
    virtual bool input(InetMessenger* m)
    {
        return true;
    }
    virtual bool output(InetMessenger* m)
    {
        return true;
    }
    virtual bool error(InetMessenger* m)
    {
        return true;
    }

    typedef bool (InetReceiver::*Command)(InetMessenger* m);
};

class InetMessenger : public Messenger
{
protected:
    InetReceiver::Command   op;

private:
    int                     scopeID;
    Address*                remoteAddress;
    Address*                localAddress;
    u16                     remotePort;
    u16                     localPort;
    int                     code;

public:
    InetMessenger(InetReceiver::Command op = 0,
                  void* chunk = 0, long len = 0, long pos = 0) :
        Messenger(chunk, len, pos),
        op(op),
        remoteAddress(0),
        localAddress(0),
        remotePort(0),
        localPort(0),
        code(0)
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
                return (receiver->*op)(this);
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
        return im->getLocal();
    }
};

class InetRemoteAddressAccessor : public Accessor
{
public:
    void* getKey(Messenger* m)
    {
        InetMessenger* im = dynamic_cast<InetMessenger*>(m);
        ASSERT(im);
        return im->getRemote();
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
