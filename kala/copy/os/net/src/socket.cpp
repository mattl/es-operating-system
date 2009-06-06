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

#include <new>
#include <stdlib.h>
#include <string.h>
#include <es/net/arp.h>
#include "dix.h"
#include "inet4address.h"
#include "loopback.h"
#include "socket.h"
#include "visualizer.h"

es::Resolver*          Socket::resolver = 0;
es::InternetConfig*    Socket::config = 0;
es::Context*           Socket::interface = 0;

AddressFamily::List Socket::addressFamilyList;
NetworkInterface*   Socket::interfaces[Socket::INTERFACE_MAX];
Timer*              Socket::timer;

LoopbackAccessor    LoopbackInterface::loopbackAccessor;

void Socket::
initialize()
{
    DateTime seed = DateTime::getNow();
    srand48(seed.getTicks());
    timer = new Timer;
}

int Socket::
addInterface(es::NetworkInterface* networkInterface)
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

    switch (networkInterface->getType())
    {
      case es::NetworkInterface::Ethernet:
      {
        DIXInterface* dixInterface = new DIXInterface(networkInterface);
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
      case es::NetworkInterface::Loopback:
      {
        LoopbackInterface* loopbackInterface = new LoopbackInterface(networkInterface);
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

void Socket::
removeInterface(es::NetworkInterface* networkInterface)
{
    for (int n = 1; n < Socket::INTERFACE_MAX; ++n)
    {
        NetworkInterface* i = interfaces[n];
        if (i && i->networkInterface == networkInterface)
        {
            interfaces[n] = 0;
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
    af(0),
    recvBufferSize(8192),
    sendBufferSize(8192),
    errorCode(0),
    selector(0),
    blocking(true)
{
    af = getAddressFamily(family);
}

Socket::
~Socket()
{
    // Leave from multicast groups XXX

    if (adapter)
    {
        SocketUninstaller uninstaller(this);
        adapter->accept(&uninstaller);
    }
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
    return getLocalPort();
}

bool Socket::
isClosed()
{
    return isBound() && !getAdapter();    // bound and then uninstalled?
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

long long Socket::
getTimeout()
{
    return timeout;
}

void Socket::
setTimeout(long long timeSpan)
{
    timeout = timeSpan;
}

bool Socket::
getReuseAddress()
{
}

void Socket::
setReuseAddress(bool on)
{
}

void Socket::
bind(es::InternetAddress* addr, int port)
{
    if (isBound())
    {
        return;
    }

    if (port == 0)
    {
        port = af->selectEphemeralPort(this);
        if (port == 0)
        {
            return;
        }
        // XXX Reserve anon from others
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
connect(es::InternetAddress* addr, int port)
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

    es::InternetAddress* src = 0;
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
    int code = m.getErrorCode();
    if (code != EINPROGRESS)
    {
        errorCode = code;
    }
}

es::Socket* Socket::
accept()
{
    SocketMessenger m(this, &SocketReceiver::accept);
    Visitor v(&m);
    adapter->accept(&v);
    errorCode = m.getErrorCode();
    int code = m.getErrorCode();
    if (code != EAGAIN)
    {
        errorCode = code;
    }
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
    int code = m.getErrorCode();
    if (code != EAGAIN)
    {
        errorCode = code;
    }
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
        errorCode = ENOTCONN;
        return -errorCode;
    }

    SocketMessenger m(this, &SocketReceiver::read, dst, count);
    Visitor v(&m);
    adapter->accept(&v);
    int code = m.getErrorCode();
    if (code)
    {
        if (code != EAGAIN)
        {
            errorCode = code;
        }
        return -errorCode;
    }
    return m.getLength();
}

int Socket::
recvFrom(void* dst, int count, int flags, es::InternetAddress** addr, int* port)
{
    if (!adapter)
    {
        errorCode = ENOTCONN;
        return -errorCode;
    }

    SocketMessenger m(this, &SocketReceiver::read, dst, count);
    Visitor v(&m);
    adapter->accept(&v);
    int code = m.getErrorCode();
    if (code)
    {
        if (code != EAGAIN)
        {
            errorCode = code;
        }
        return -errorCode;
    }
    if (addr)
    {
        *addr = m.getRemote();
    }
    if (port)
    {
        *port = m.getRemotePort();
    }
    return m.getLength();
}

int Socket::
sendTo(const void* src, int count, int flags, es::InternetAddress* addr, int port)
{
    if (!adapter)
    {
        errorCode = ENOTCONN;
        return -errorCode;
    }

    SocketMessenger m(this, &SocketReceiver::write, const_cast<void*>(src), count);

    m.setRemote(dynamic_cast<Inet4Address*>(addr));
    m.setRemotePort(port);

    Visitor v(&m);
    adapter->accept(&v);
    int code = m.getErrorCode();
    if (code)
    {
        if (code != EAGAIN)
        {
            errorCode = code;
        }
        return -errorCode;
    }
    return m.getLength();
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
    errorCode = m.getErrorCode();
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
    errorCode = m.getErrorCode();
}

int Socket::
write(const void* src, int count)
{
    if (!adapter)
    {
        errorCode = ENOTCONN;
        return -errorCode;
    }

    SocketMessenger m(this, &SocketReceiver::write, const_cast<void*>(src), count);
    Visitor v(&m);
    adapter->accept(&v);
    int code = m.getErrorCode();
    if (code)
    {
        if (code != EAGAIN)
        {
            errorCode = code;
        }
        return -errorCode;
    }
    return m.getLength();
}

bool Socket::
isAcceptable()
{
    if (!adapter)
    {
        return false;
    }
    SocketMessenger m(this, &SocketReceiver::isAcceptable);
    Visitor v(&m);
    adapter->accept(&v);
    return m.getErrorCode();
}

bool Socket::
isConnectable()
{
    if (!adapter)
    {
        return false;
    }
    SocketMessenger m(this, &SocketReceiver::isConnectable);
    Visitor v(&m);
    adapter->accept(&v);
    return m.getErrorCode();
}

bool Socket::
isReadable()
{
    if (!adapter)
    {
        return false;
    }
    SocketMessenger m(this, &SocketReceiver::isReadable);
    Visitor v(&m);
    adapter->accept(&v);
    return m.getErrorCode();
}

bool Socket::
isWritable()
{
    if (!adapter)
    {
        return false;
    }
    SocketMessenger m(this, &SocketReceiver::isWritable);
    Visitor v(&m);
    adapter->accept(&v);
    return m.getErrorCode();
}

//
// IMulticastSocket
//

bool Socket::
getLoopbackMode()
{
}

void Socket::
setLoopbackMode(bool disable)
{
}

void Socket::
joinGroup(es::InternetAddress* addr)
{
    Address* address = static_cast<Address*>(addr);
    if (address->isMulticast())
    {
        addresses.addLast(address);
        address->addSocket(this);
    }
}

void Socket::
leaveGroup(es::InternetAddress* addr)
{
    Address* address = static_cast<Address*>(addr);
    if (addresses.contains(address))
    {
        addresses.remove(address);
        address->removeSocket(this);
    }
}

void Socket::
notify()
{
    if (!adapter)
    {
        return;
    }

    SocketMessenger m(this, &SocketReceiver::notify);
    Visitor v(&m);
    adapter->accept(&v);
}

int Socket::
add(es::Monitor* selector)
{
    esReport("Socket::%s(%p) : %p\n", __func__, selector, this->selector);
    if (!selector || this->selector)
    {
        return -1;
    }

    selector->addRef();
    this->selector = selector;
    return 1;
}

int Socket::
remove(es::Monitor* selector)
{
    esReport("Socket::%s(%p) : %p\n", __func__, selector, this->selector);
    if (!selector || selector != this->selector)
    {
        return -1;
    }

    selector->release();
    this->selector = 0;
    return 1;
}

//
// IInterface
//

Object* Socket::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Socket::iid()) == 0)
    {
        objectPtr = static_cast<es::Socket*>(this);
    }
    else if (strcmp(riid, es::Selectable::iid()) == 0)
    {
        objectPtr = static_cast<es::Selectable*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Socket*>(this);
    }
    else if (strcmp(riid, es::MulticastSocket::iid()) == 0 && type == es::Socket::Datagram)
    {
        objectPtr = static_cast<es::Socket*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Socket::
addRef()
{
    return ref.addRef();
}

unsigned int Socket::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
