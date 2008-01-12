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

#include <es/net/inet4.h>
#include "datagram.h"

bool DatagramReceiver::
input(InetMessenger* m, Conduit* c)
{
    esReport("DatagramReceiver::input\n");

    long len = m->getLength();

    RingHdr ringhdr(len);

    // Copy-in data
    Synchronized<IMonitor*> method(monitor);
    if (recvRing.getUnused() < sizeof ringhdr + len)
    {
        // Ins. space in recvRing.
        // XXX record error code.
        return true;
    }
    recvRing.write(&ringhdr, sizeof ringhdr);
    recvRing.write(m->fix(len), len);
    monitor->notifyAll();
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
    Synchronized<IMonitor*> method(monitor);

    esReport("DatagramReceiver::error()\n");
    errorCode = m->getErrorCode();
    monitor->notifyAll();
    return true;
}

bool DatagramReceiver::
read(SocketMessenger* m, Conduit* c)
{
    Synchronized<IMonitor*> method(monitor);

    esReport("DatagramReceiver::read()\n");

    Socket* socket = getSocket();
    ASSERT(socket);

    // Copy-out data
    RingHdr ringhdr;
    long len;
    while ((len = recvRing.peek(&ringhdr, sizeof ringhdr)) == 0 && errorCode == 0)
    {
        if (!monitor->wait(socket->getTimeout()))
        {
            if (errorCode == 0)
            {
                errorCode = ETIMEDOUT;
            }
        }
    }

    if (len < 0)
    {
        return false;
    }

    if (errorCode)
    {
        m->setErrorCode(errorCode);
        errorCode = 0;
        m->setPosition(m->getSize());
        return false;
    }

    recvRing.read(&ringhdr, sizeof ringhdr);
    ASSERT(ringhdr.len <= m->getSize());    // XXX
    m->setSize(ringhdr.len);
    recvRing.read(m->fix(ringhdr.len), ringhdr.len);
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
    SocketUninstaller uninstaller(getSocket());
    conduit->getB()->accept(&uninstaller);
    return false;
}

bool DatagramReceiver::
notify(SocketMessenger* m, Conduit* c)
{
    Synchronized<IMonitor*> method(monitor);

    errorCode = ETIMEDOUT;
    monitor->notifyAll();
    return false;
}
