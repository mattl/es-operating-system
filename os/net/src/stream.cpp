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

#include <es/endian.h>
#include <es/handle.h>
#include <es/md5.h>
#include <es/net/inet4.h>
#include "stream.h"

const TimeSpan StreamReceiver::R2(1000000000LL);    // RFC 1122 4.2.3.5
const TimeSpan StreamReceiver::R2_SYN(1800000000LL);
const TimeSpan StreamReceiver::MSL(1200000000LL);
const TimeSpan StreamReceiver::RTT_MIN(10000000);
const TimeSpan StreamReceiver::RTT_MAX(2 * MSL);
const TimeSpan StreamReceiver::RTT_DEFAULT(30000000);
const TimeSpan StreamReceiver::PERSIST_MAX(MAX_BACKOFF * RTT_MAX);
const TimeSpan StreamReceiver::DACK_TIMEOUT(2000000);

StreamReceiver::StateClosed      StreamReceiver::stateClosed;
StreamReceiver::StateListen      StreamReceiver::stateListen;
StreamReceiver::StateSynSent     StreamReceiver::stateSynSent;
StreamReceiver::StateSynReceived StreamReceiver::stateSynReceived;
StreamReceiver::StateEstablished StreamReceiver::stateEstablished;
StreamReceiver::StateFinWait1    StreamReceiver::stateFinWait1;
StreamReceiver::StateFinWait2    StreamReceiver::stateFinWait2;
StreamReceiver::StateCloseWait   StreamReceiver::stateCloseWait;
StreamReceiver::StateLastAck     StreamReceiver::stateLastAck;
StreamReceiver::StateClosing     StreamReceiver::stateClosing;
StreamReceiver::StateTimeWait    StreamReceiver::stateTimeWait;

// cf. RFC 1948
TCPSeq StreamReceiver::
isn(InetMessenger* m)
{
    MD5Context      context;
    u8              bytes[16];
    Handle<Address> address;
    int             len;
    u16             port;

    MD5Init(&context);

    // XXX MD5Update(&context, per-host secret and the boot);

    address = m->getRemote();
    len = address->getAddress(bytes, sizeof bytes);
    MD5Update(&context, bytes, len);
    port = htons(m->getLocalPort());
    MD5Update(&context, &port, 2);

    address = m->getLocal();
    len = address->getAddress(bytes, sizeof bytes);
    MD5Update(&context, bytes, len);
    port = htons(m->getRemotePort());
    MD5Update(&context, &port, 2);

    MD5Final(bytes, &context);

    s32 isn = *(s32*) &bytes[0];
    isn ^= *(s32*) &bytes[4];
    isn ^= *(s32*) &bytes[8];
    isn ^= *(s32*) &bytes[12];
    isn += (s32) (DateTime::getNow().getTicks() / (10000000 / IIS_CLOCK));  // XXX resolution
    return TCPSeq(isn);
}

s32 StreamReceiver::
getDefaultMSS()
{
    Socket* socket = getSocket();
    if (socket)
    {
        switch (socket->getAddressFamily())
        {
          case AF_INET:
            return IP_MIN_MTU - sizeof(IPHdr) - sizeof(TCPHdr);
          case AF_INET6:
            return IP6_MIN_MTU - sizeof(IP6Hdr) - sizeof(TCPHdr);
        }
    }
    return 0;
}

bool StreamReceiver::
input(InetMessenger* m, Conduit* c)
{
    esReport("StreamReceiver::input %s\n", state->getName());
    if (state->input(m, this))
    {
        int size = 14 + 60 + 60 + mss;  // XXX Assume MAC, IPv4, TCP
        Handle<InetMessenger> seg = new InetMessenger(&InetReceiver::output, size, size);
        Handle<Address> addr;
        seg->setLocal(addr = m->getLocal());
        seg->setRemote(addr = m->getRemote());
        seg->setLocalPort(m->getLocalPort());
        seg->setRemotePort(m->getRemotePort());
        seg->setType(IPPROTO_TCP);
        Visitor v(seg);
        conduit->accept(&v, conduit->getB());
    }
    return true;
}

bool StreamReceiver::
output(InetMessenger* m, Conduit* c)
{
    hole = 0;
    onxt = sendNext;
    return state->output(m, this);
}

bool StreamReceiver::
error(InetMessenger* m, Conduit* c)
{
    // monitor->notifyAll();
    return state->error(m, this);
}

void StreamReceiver::
abort()
{
    setState(stateClosed);
    stopRxmitTimer();
    stopAckTimer();

    // XXX process listen and completed, etc.

    monitor->notifyAll();

    Socket* socket = getSocket();
    SocketUninstaller uninstaller(socket);
    Adapter* adapter = socket->getAdapter();
    if (adapter)
    {
        socket->setAdapter(0);
        adapter->accept(&uninstaller);
    }
}

// Copy data out of recvRing
bool StreamReceiver::
read(SocketMessenger* m, Conduit* c)
{
    Synchronized<IMonitor*> method(monitor);

    long len;
    while ((len = recvRing.getUsed()) == 0 && !isShutdownInput())
    {
        monitor->wait();
    }
    if (len < 0)
    {
        return false;
    }

    if (m->getSize() < len)
    {
        len = m->getSize();
    }
    m->movePosition(-len);
    recvRing.read(m->fix(len), len);

    int size = 14 + 60 + 60 + mss;  // XXX Assume MAC, IPv4, TCP
    Handle<InetMessenger> seg = new InetMessenger(&InetReceiver::output, size, size);
    Handle<Address> addr;
    seg->setLocal(addr = m->getLocal());
    seg->setRemote(addr = m->getRemote());
    seg->setLocalPort(m->getLocalPort());
    seg->setRemotePort(m->getRemotePort());
    seg->setType(IPPROTO_TCP);
    Visitor v(seg);
    conduit->accept(&v, conduit->getB());

    return false;
}

// Copy data into sendRing
bool StreamReceiver::
write(SocketMessenger* m, Conduit* c)
{
    Synchronized<IMonitor*> method(monitor);
    long len;
    while ((len = sizeof sendBuf - sendRing.getUsed()) == 0 && !isShutdownOutput())
    {
        monitor->wait();
    }
    if (len < 0)
    {
        return false;
    }

    if (m->getLength() < len)
    {
        len = m->getLength();
    }
    sendRing.write(m->fix(len), len);
    m->setPosition(m->getSize() - len);

    int size = 14 + 60 + 60 + mss;  // XXX Assume MAC, IPv4, TCP
    Handle<InetMessenger> seg = new InetMessenger(&InetReceiver::output, size, size);
    Handle<Address> addr;
    seg->setLocal(addr = m->getLocal());
    seg->setRemote(addr = m->getRemote());
    seg->setLocalPort(m->getLocalPort());
    seg->setRemotePort(m->getRemotePort());
    seg->setType(IPPROTO_TCP);
    Visitor v(seg);
    conduit->accept(&v, conduit->getB());

    return false;
}

bool StreamReceiver::
accept(SocketMessenger* m, Conduit* c)
{
    return state->accept(m, this);
}

bool StreamReceiver::
listen(SocketMessenger* m, Conduit* c)
{
    return false;
}

bool StreamReceiver::
connect(SocketMessenger* m, Conduit* c)
{
    return state->connect(m, this);
}

bool StreamReceiver::
close(SocketMessenger* m, Conduit* c)
{
    shutrd = shutwr = true;
    if (state->close(m, this))
    {
        int size = 14 + 60 + 60 + mss;  // XXX Assume MAC, IPv4, TCP
        Handle<InetMessenger> seg = new InetMessenger(&InetReceiver::output, size, size);
        Handle<Address> addr;
        seg->setLocal(addr = m->getLocal());
        seg->setRemote(addr = m->getRemote());
        seg->setLocalPort(m->getLocalPort());
        seg->setRemotePort(m->getRemotePort());
        seg->setType(IPPROTO_TCP);
        Visitor v(seg);
        conduit->accept(&v, conduit->getB());
    }
    monitor->notifyAll();
    return true;
}

bool StreamReceiver::
shutdownOutput(SocketMessenger* m, Conduit* c)
{
    shutwr = true;
    if (state->close(m, this))
    {
        int size = 14 + 60 + 60 + mss;  // XXX Assume MAC, IPv4, TCP
        Handle<InetMessenger> seg = new InetMessenger(&InetReceiver::output, size, size);
        Handle<Address> addr;
        seg->setLocal(addr = m->getLocal());
        seg->setRemote(addr = m->getRemote());
        seg->setLocalPort(m->getLocalPort());
        seg->setRemotePort(m->getRemotePort());
        seg->setType(IPPROTO_TCP);
        Visitor v(seg);
        conduit->accept(&v, conduit->getB());
    }
    return false;
}

bool StreamReceiver::
shutdownInput(SocketMessenger* m, Conduit* c)
{
    shutrd = true;
    monitor->notifyAll();
    return false;
}

bool StreamReceiver::
StateClosed::connect(SocketMessenger* m, StreamReceiver* s)
{
    Synchronized<IMonitor*> method(s->monitor);

    // Initialize tcp
    s->initRto();

    s->iss = s->isn(m);
    s->sendUna = s->iss;
    s->sendNext = s->iss; // incremented later by output()
    s->sendMax = s->iss;
    s->sendUp = s->sendUna;

    s->sendFack = s->sendUna;
    s->rxmitData = 0;
    s->sendAwin = 0;

#ifdef TCP_SACK
    s->sendRecover = s->sendUna;
    s->lastSack = s->sendUna;
#endif  // TCP_SACK

    // ++interface->tcpStat.activeOpens;

    s->setState(stateSynSent);

    // Send SYN
    int size = 14 + 60 + 60;      // XXX Assume MAC, IPv4, TCP
    Handle<InetMessenger> seg = new InetMessenger(&InetReceiver::output, size, size);
    Handle<Address> addr;
    seg->setLocal(addr = m->getLocal());
    seg->setRemote(addr = m->getRemote());
    seg->setLocalPort(m->getLocalPort());
    seg->setRemotePort(m->getRemotePort());
    seg->setType(IPPROTO_TCP);
    Visitor v(seg);
    s->conduit->accept(&v, s->conduit->getB());

    while (s->state == &stateSynSent)
    {
        s->monitor->wait();
    }

    return false;
}

bool StreamReceiver::
StateListen::accept(SocketMessenger* m, StreamReceiver* s)
{
    Synchronized<IMonitor*> method(s->monitor);

    while (s->state == &stateListen && s->accepted.isEmpty())
    {
        s->monitor->wait();
    }
    if (s->state == &stateListen && !s->accepted.isEmpty())
    {
        StreamReceiver* accepted = s->accepted.removeFirst();
        ASSERT(accepted);
        m->setSocket(accepted->getSocket());
    }
    return false;
}
