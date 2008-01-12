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

#include <new>
#include <stdlib.h>
#include <string.h>
#include <es/net/arp.h>
#include "dix.h"
#include "inet4address.h"
#include "loopback.h"
#include "socket.h"
#include "visualizer.h"

AddressFamily::List Socket::addressFamilyList;
Interface*          Socket::interfaces[Socket::INTERFACE_MAX];
Timer*              Socket::timer;

LoopbackAccessor    LoopbackInterface::loopbackAccessor;

void Socket::
initialize()
{
    DateTime seed = DateTime::getNow();
    srand48(seed.getTicks());
    timer = new Timer;
}

int Socket::addInterface(IStream* stream, int hrd)
{
    int n;
    for (n = 1; n < Socket::INTERFACE_MAX; ++n)
    {
        if (interfaces[n] == 0)
        {
            break;
        }
    }
    if (Socket::INTERFACE_MAX <= n)
    {
        return -1;
    }

    switch (hrd)
    {
      case ARPHdr::HRD_ETHERNET:
      {
        DIXInterface* dixInterface = new DIXInterface(stream);
        dixInterface->setScopeID(n);
        interfaces[n] = dixInterface;

        // Connect address families to the loopback interface.
        AddressFamily* af;
        AddressFamily::List::Iterator iter = addressFamilyList.begin();
        while ((af = iter.next()))
        {
            af->addInterface(dixInterface);
        }

#if 0
        {
            Visualizer v;
            dixInterface->getAdapter()->accept(&v);
        }
#endif

        dixInterface->start();
        break;
      }
      case ARPHdr::HRD_LOOPBACK:
      {
        LoopbackInterface* loopbackInterface = new LoopbackInterface(stream);
        loopbackInterface->setScopeID(n);
        interfaces[n] = loopbackInterface;

        // Connect address families to the loopback interface.
        AddressFamily* af;
        AddressFamily::List::Iterator iter = addressFamilyList.begin();
        while ((af = iter.next()))
        {
            af->addInterface(loopbackInterface);
        }

#if 0
        {
            Visualizer v;
            loopbackInterface->getAdapter()->accept(&v);
        }
#endif

        loopbackInterface->start();
        break;
      }
      default:
        return -1;
    }
    return n;
}

void Socket::removeInterface(IStream* stream)
{
    int n;
    for (n = 1; n < Socket::INTERFACE_MAX; ++n)
    {
        Interface* i = interfaces[n];
        if (i && i->stream == stream)
        {
            interfaces[0] = 0;
            delete i;
            break;
        }
    }
}

Socket::
Socket(int family, int type, int protocol) :
    family(family),
    type(type),
    protocol(protocol),
    adapter(0),
    recvBufferSize(8192),
    sendBufferSize(8192)
{
    af = getAddressFamily(family);
}

Socket::
~Socket()
{
    // Leave from multicast groups XXX

}

bool Socket::
input(InetMessenger* m, Conduit* c)
{
    return true;
}

bool Socket::
output(InetMessenger* m, Conduit* c)
{
    return true;
}

bool Socket::
error(InetMessenger* m, Conduit* c)
{
    return true;
}

//
// ISocket
//

bool Socket::
isBound()
{
    return getAdapter();    // installed?
}

bool Socket::
isClosed()
{
}

bool Socket::
isConnected()
{
    return getRemotePort();
}

int Socket::
getHops()
{
}

void Socket::
setHops(int limit)
{
}

int Socket::
getReceiveBufferSize()
{
    return recvBufferSize;
}

void Socket::
setReceiveBufferSize(int size)
{
    if (!isBound())
    {
        recvBufferSize = size;
    }
}

int Socket::
getSendBufferSize()
{
    return sendBufferSize;
}

void Socket::
setSendBufferSize(int size)
{
    if (!isBound())
    {
        sendBufferSize = size;
    }
}

bool Socket::
isReuseAddress()
{
}

void Socket::
setReuseAddress(bool on)
{
}

void Socket::
bind(IInternetAddress* addr, int port)
{
    if (isBound())
    {
        return;
    }

    Conduit* protocol = af->getProtocol(this);
    if (!protocol)
    {
        return;
    }

    setLocal(dynamic_cast<Address*>(addr));   // XXX
    setLocalPort(port);

    SocketInstaller installer(this);
    protocol->accept(&installer);
}

void Socket::
connect(IInternetAddress* addr, int port)
{
    if (isConnected() || addr == 0 || port == 0)
    {
        return;
    }

    Conduit* protocol = af->getProtocol(this);
    if (!protocol)
    {
        return;
    }

    int anon = 0;
    if (getLocalPort() == 0)
    {
        anon = af->selectEphemeralPort(this);
        if (anon == 0)
        {
            return;
        }
        // XXX Reserve anon from others
    }

    IInternetAddress* src = 0;
    if (!getLocal())    // XXX any
    {
        src = af->selectSourceAddress(addr);
        if (!src)
        {
            return;
        }
    }

    if (isBound())
    {
        SocketDisconnector disconnector(this);
        adapter->accept(&disconnector);

        if (anon)
        {
            setLocalPort(anon);
        }
        if (src)
        {
            setLocal(dynamic_cast<Address*>(src));
        }

        setRemote(dynamic_cast<Address*>(addr));   // XXX
        setRemotePort(port);

        SocketConnector connector(this, disconnector.getProtocol());
        protocol->accept(&connector);
    }
    else
    {
        if (anon)
        {
            setLocalPort(anon);
        }
        if (src)
        {
            setLocal(dynamic_cast<Address*>(src));
        }

        setRemote(dynamic_cast<Address*>(addr));   // XXX
        setRemotePort(port);

        SocketInstaller installer(this);
        protocol->accept(&installer);
    }

    // Request connect
    SocketMessenger m(this, &SocketReceiver::connect);
    Visitor v(&m);
    adapter->accept(&v);
}

ISocket* Socket::
accept()
{
    SocketMessenger m(this, &SocketReceiver::accept);
    Visitor v(&m);
    adapter->accept(&v);
    return m.getSocket();
}

void Socket::
close()
{
    if (!adapter)
    {
        return;
    }

    SocketMessenger m(this, &SocketReceiver::close);
    Visitor v(&m);
    adapter->accept(&v);
}

void Socket::
listen(int backlog)
{
    StreamReceiver* s = dynamic_cast<StreamReceiver*>(getReceiver());
    if (s)
    {
        s->setState(StreamReceiver::stateListen);
    }
}

int Socket::
read(void* dst, int count)
{
    if (!adapter)
    {
        return -1;
    }

    SocketMessenger m(this, &SocketReceiver::read, dst, count);
    Visitor v(&m);
    adapter->accept(&v);
    return m.getLength();
}

int Socket::
recvFrom(void* dst, int count, int flags, IInternetAddress** addr, int* port)
{
}

int Socket::
sendTo(const void* src, int count, int flags, IInternetAddress* addr, int port)
{
}

void Socket::
shutdownInput()
{
    if (!adapter)
    {
        return;
    }

    SocketMessenger m(this, &SocketReceiver::shutdownInput);
    Visitor v(&m);
    adapter->accept(&v);
}

void Socket::
shutdownOutput()
{
    if (!adapter)
    {
        return;
    }

    SocketMessenger m(this, &SocketReceiver::shutdownOutput);
    Visitor v(&m);
    adapter->accept(&v);
}

int Socket::
write(const void* src, int count)
{
    if (!adapter)
    {
        return -1;
    }

    SocketMessenger m(this, &SocketReceiver::write, const_cast<void*>(src), count);
    Visitor v(&m);
    adapter->accept(&v);
    return m.getLength();
}

//
// IMulticastSocket
//

int Socket::
getLoopbackMode()
{
}

void Socket::
setLoopbackMode(bool disable)
{
}

void Socket::
joinGroup(IInternetAddress* addr)
{
    Address* address = static_cast<Address*>(addr);
    if (address->isMulticast())
    {
        addresses.addLast(address);
        address->addSocket(this);
    }
}

void Socket::
leaveGroup(IInternetAddress* addr)
{
    Address* address = static_cast<Address*>(addr);
    if (addresses.contains(address))
    {
        addresses.remove(address);
        address->removeSocket(this);
    }
}

//
// IInterface
//

bool Socket::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_ISocket)
    {
        *objectPtr = static_cast<ISocket*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<ISocket*>(this);
    }
    else if (riid == IID_IMulticastSocket && type == ISocket::Datagram)
    {
        *objectPtr = static_cast<Socket*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Socket::
addRef(void)
{
    return ref.addRef();
}

unsigned int Socket::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
