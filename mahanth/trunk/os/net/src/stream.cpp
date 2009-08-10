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

int StreamReceiver::
getDefaultMSS()
{
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

int StreamReceiver::
getDefaultMSS(int mtu)
{
    if (socket)
    {
        switch (socket->getAddressFamily())
        {
          case AF_INET:
            return mtu - sizeof(IPHdr) - sizeof(TCPHdr);
          case AF_INET6:
            return mtu - sizeof(IP6Hdr) - sizeof(TCPHdr);
        }
    }
    return 0;
}

bool StreamReceiver::
input(InetMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

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
    Synchronized<es::Monitor*> method(monitor);

    hole = 0;
    onxt = sendNext;
    return state->output(m, this);
}

bool StreamReceiver::
error(InetMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

    int code = m->getErrorCode();
    if (code != ENETUNREACH)
    {
        err = code;
        abort();
    }
    return true;
}

void StreamReceiver::
abort()
{
    setState(stateClosed);
    stopRxmitTimer();
    stopAckTimer();

    // XXX process listen and completed, etc.

    notify();
}

// Copy data out of recvRing
bool StreamReceiver::
read(SocketMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

    while (!isReadable())
    {
        if (!socket->getBlocking())
        {
            m->setErrorCode(EAGAIN);
            return false;
        }
        if (socket->getTimeout())
        {
            monitor->wait(socket->getTimeout());

            if (!isReadable())
            {
                m->setErrorCode(ETIMEDOUT);
                return false;
            }
            else
            {
                break;
            }
        }
        monitor->wait();
    }

    long len;
    if (recvRing.getUsed() + recvUp - recvNext == 0)
    {
       len = 1;
       hadUrg = true;
    }
    else if (!hadUrg && haveUrg)
    {
        len = std::min(recvRing.getUsed(), recvRing.getUsed() + recvUp - recvNext);
    }
    else
    {
        len = recvRing.getUsed();
    }
    if (len < 0)
    {
        // XXX m->setErrorCode(XXX);
        return false;
    }

    if (m->getSize() < len)
    {
        len = m->getSize();
    }
    m->setSize(len);
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
    Synchronized<es::Monitor*> method(monitor);
    while (!isWritable())
    {
        if (!socket->getBlocking())
        {
            m->setErrorCode(EAGAIN);
            return false;
        }
        if (socket->getTimeout())
        {
            monitor->wait(socket->getTimeout());

            if (!isWritable())
            {
                m->setErrorCode(ETIMEDOUT);
                return false;
            }
            else
            {
                break;
            }
        }
        monitor->wait();
    }

    long len = socket->getSendBufferSize() - sendRing.getUsed();
    if (len < 0)
    {
        // XXX m->setErrorCode(XXX);
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
    seg->setFlag(m->getFlag());
    Visitor v(seg);
    conduit->accept(&v, conduit->getB());

    return false;
}

bool StreamReceiver::
accept(SocketMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

    return state->accept(m, this);
}

bool StreamReceiver::
listen(SocketMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

    return false;
}

bool StreamReceiver::
connect(SocketMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

    return state->connect(m, this);
}

bool StreamReceiver::
close(SocketMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

    if (socket->getTimeout() == 0 && !socket->getBlocking())
    {
        state->abort(this);
        return false;
    }

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

    while (!isClosable())
    {
        ASSERT(socket);
        if (!socket->getBlocking())
        {
            m->setErrorCode(EAGAIN);
            return false;
        }
        monitor->wait();
    }
    return false;
}

bool StreamReceiver::
shutdownOutput(SocketMessenger* m, Conduit* c)
{
    Synchronized<es::Monitor*> method(monitor);

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
    Synchronized<es::Monitor*> method(monitor);

    shutrd = true;
    notify();
    return false;
}

bool StreamReceiver::
StateClosed::connect(SocketMessenger* m, StreamReceiver* s)
{
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

    Handle<Address> local = m->getLocal();
    s->mss = s->getDefaultMSS(local->getPathMTU());

    s->setState(stateSynSent);

    // Send SYN
    int size = 14 + 60 + 60;      // XXX Assume MAC, IPv4, TCP
    Handle<InetMessenger> seg = new InetMessenger(&InetReceiver::output, size, size);
    seg->setLocal(local);
    seg->setRemote(Handle<Address>(m->getRemote()));
    seg->setLocalPort(m->getLocalPort());
    seg->setRemotePort(m->getRemotePort());
    seg->setType(IPPROTO_TCP);
    Visitor v(seg);
    s->conduit->accept(&v, s->conduit->getB());

    while (!s->isConnectable())
    {
        if (!s->socket->getBlocking())
        {
            m->setErrorCode(EINPROGRESS);
            return false;
        }
        s->monitor->wait();
    }

    return false;
}

bool StreamReceiver::
StateListen::accept(SocketMessenger* m, StreamReceiver* s)
{
    while (!s->isAcceptable())
    {
        if (!s->socket->getBlocking())
        {
            m->setErrorCode(EAGAIN);
            return false;
        }
        s->monitor->wait();
    }
    if (s->state == &stateListen && !s->accepted.isEmpty())
    {
        StreamReceiver* accepted = s->accepted.removeFirst();
        ASSERT(accepted);
        ASSERT(accepted->socket);
        m->setSocket(accepted->socket);
    }
    return false;
}
