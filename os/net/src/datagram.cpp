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

#include <es/net/inet4.h>
#include "datagram.h"

bool DatagramReceiver::
input(InetMessenger* m)
{
    long len = m->getLength();

    RingHdr ringhdr(len);

    // Copy-in data
    Synchronized<IMonitor*> method(monitor);
    if (sizeof recvBuf - recvRing.getUsed() < sizeof ringhdr + len)
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
output(InetMessenger* m)
{
    return true;
}

bool DatagramReceiver::
error(InetMessenger* m)
{
    monitor->notifyAll();
    return true;
}

bool DatagramReceiver::
read(SocketMessenger* m)
{
    Synchronized<IMonitor*> method(monitor);

    // Copy-out data
    RingHdr ringhdr;
    long len;
    while ((len = recvRing.peek(&ringhdr, sizeof ringhdr)) == 0)
    {
        monitor->wait();
    }
    if (len < 0)
    {
        return false;
    }

    recvRing.read(&ringhdr, sizeof ringhdr);
    ASSERT(ringhdr.len <= m->getSize());    // XXX
    m->setPosition(m->getSize() - ringhdr.len);
    recvRing.read(m->fix(ringhdr.len), ringhdr.len);
    return false;
}

bool DatagramReceiver::
write(SocketMessenger* m)
{
    m->setCommand(&InetReceiver::output);
    return true;
}

bool DatagramReceiver::
close(SocketMessenger* m)
{
    SocketUninstaller uninstaller(getSocket());
    conduit->getB()->accept(&uninstaller);
    return false;
}
