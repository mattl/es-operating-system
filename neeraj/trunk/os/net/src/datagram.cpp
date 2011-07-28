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

#include <es/net/inet4.h>
#include "datagram.h"

bool DatagramReceiver::
input(InetMessenger* m, Conduit* c)
{
    esReport("DatagramReceiver::input\n");

    long len = m->getLength();

    RingHdr ringhdr(len, m->getRemote(), m->getRemotePort());

    // Copy-in data
    Synchronized<es::Monitor*> method(monitor);
    if (recvRing.getUnused() < sizeof ringhdr + len)
    {
        // Ins. space in recvRing.
        // XXX record error code.
        return true;
    }
    recvRing.write(&ringhdr, sizeof ringhdr);
    recvRing.write(m->fix(len), len);
    notify();
    return true;
}

bool DatagramReceiver::
output(InetMessenger* m, Conduit* c)
{
    return true;
}

bool DatagramReceiver::
error(InetMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

    esReport("DatagramReceiver::error()\n");
    errorCode = m->getErrorCode();
    notify();
    return true;
}

bool DatagramReceiver::
read(SocketMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

    esReport("DatagramReceiver::read()\n");

    ASSERT(socket);

    // Copy-out data
    while (!isReadable())
    {
        if (!socket->getBlocking())
        {
            m->setErrorCode(errorCode ? errorCode : EAGAIN);
            return false;
        }
        if (!monitor->wait(socket->getTimeout()))
        {
            if (errorCode == 0)
            {
                errorCode = ETIMEDOUT;
            }
        }
    }

    if (errorCode)
    {
        m->setErrorCode(errorCode);
        errorCode = 0;
        m->setPosition(m->getSize());
        return false;
    }

    RingHdr ringhdr;
    long len = recvRing.read(&ringhdr, sizeof ringhdr);
    ASSERT(len == sizeof ringhdr);
    len = ringhdr.len;
    m->setRemote(ringhdr.addr);
    m->setRemotePort(ringhdr.port);
    ringhdr.addr->release();
    if (len <= m->getSize())
    {
        m->setSize(len);
        recvRing.read(m->fix(len), len);
    }
    else
    {
        recvRing.read(m->fix(m->getSize()), m->getSize());
        recvRing.skip(len - m->getSize());
    }
    return false;
}

bool DatagramReceiver::
write(SocketMessenger* m, Conduit* c)
{
    int pos = 14 + 60 + 60;  // XXX Assume MAC, IPv4
    int len = m->getSize();
    Handle<InetMessenger> d = new InetMessenger(&InetReceiver::output, pos + len, pos);
    memmove(d->fix(len), m->fix(len), len);

    Handle<Address> addr;
    d->setLocal(addr = m->getLocal());
    d->setRemote(addr = m->getRemote());
    d->setLocalPort(m->getLocalPort());
    d->setRemotePort(m->getRemotePort());
    d->setType(IPPROTO_UDP);
    Visitor v(d);
    conduit->accept(&v, conduit->getB());
    return false;
}

bool DatagramReceiver::
close(SocketMessenger* m, Conduit* c)
{
    return false;
}

bool DatagramReceiver::
notify(SocketMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

    errorCode = ETIMEDOUT;
    notify();
    return false;
}
