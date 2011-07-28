/*
 * Copyright 2008 Google Inc.
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

#include <errno.h>
#include <algorithm>
#include <es/handle.h>
#include "stream.h"

void StreamReceiver::initRto()
{
    rxmitCount = 0;
    srtt = 0;
    rttDe = RTT_DEFAULT / 4;
    rto = RTT_DEFAULT;
}

void StreamReceiver::updateRto(TimeSpan rtt)
{
    ASSERT(rttTiming != 0);
    if (srtt)
    {
        TimeSpan delta = rtt - srtt;
        srtt += delta / 8;              // g = 1/8
        if (delta < 0)
        {
            delta = -delta;
        }
        rttDe += (delta - rttDe) / 4;   // h = 1/4
    }
    else
    {
        srtt = rtt;
        rttDe = rtt / 2;
    }

    rttTiming = 0;
    rxmitCount = 0;
    rto = srtt + 4 * rttDe;
    rto = std::min(std::max(RTT_MIN, rto), RTT_MAX);
}

void StreamReceiver::startRxmitTimer()
{
    if (rxmitTimer.isEnabled())
    {
        return;
    }

    if (rxmitCount == 0)
    {
        r0 = DateTime::getNow();
    }
    Socket::alarm(&rxmitTimer, rto);
}

void StreamReceiver::stopRxmitTimer()
{
    rxmitCount = 0;
    Socket::cancel(&rxmitTimer);
}

// Reset (i.e., extend) timer using the current rxmit count.
void StreamReceiver::resetRxmitTimer()
{
    // Start alarm using the current rxmit count.
    Socket::cancel(&rxmitTimer);
    Socket::alarm(&rxmitTimer, rto);
}

// cut the threshold in half
void StreamReceiver::
cutThresh()
{
    ssThresh = (sendMax - sendUna) / 2;
    ssThresh = std::max(ssThresh, 2 * mss);
}

void StreamReceiver::
expired()
{
    esReport("expired\n");

    if (!socket)
    {
        return;
    }

    if (getState() == &stateTimeWait)
    {
        abort();
        return;
    }

    ++rxmitCount;
    if (MAX_BACKOFF <= rxmitCount)
    {
        rxmitCount = MAX_BACKOFF;
    }

    rto *= 2;
    if (RTT_MAX < rto)
    {
        rto = RTT_MAX;
    }

    if (!isPersist())
    {
        // Path MTU discovery blackhole detection [RFC 2923]
        if (PMTUD_BACKOFF == rxmitCount && state->hasBeenEstablished())
        {
            // XXX Turn off don't fragment bit to disable path MTU discovery.

            // Reduce mss to TCP default minimal
            mss = std::min(getDefaultMSS(), mss);
        }

        if (R1 == rxmitCount)
        {
            // XXX Perform gateway-reselection, ARP re-validate, etc.
        }
        else if (R1 < rxmitCount && r2 <= DateTime::getNow() - r0)
        {
            // Reached R2
            if (err == 0)
            {
                err = ETIMEDOUT;
            }
            abort();
            return;
        }
    }
    else    // In persist state
    {
        // r0 is updated everytime ACK is received by TCPStopRxmitTimer().
        if (PERSIST_MAX <= DateTime::getNow() - r0)
        {
            if (err == 0)
            {
                err = ETIMEDOUT;
            }
            abort();
            return;
        }
    }

    sendNext = sendUna;

    // Cancel RTT estimators. [Karn's algorithm]
    rttTiming = 0;

    // Goes back to slow start
    cutThresh();

    // RFC 2581 3.1 Slow Start and Congestion Avoidance
    //
    //  Furthermore, upon a timeout cwnd MUST be set to no more than the loss
    //  window, LW, which equals 1 full-sized segment (regardless of the
    //  value of IW).  Therefore, after retransmitting the dropped segment
    //  the TCP sender uses the slow start algorithm to increase the window
    //  from 1 full-sized segment to the new value of ssthresh, at which
    //  point congestion avoidance again takes over.
    //
    // We must *NOT* set cWin to IW(info).
    cWin = mss;

    // RFC 3782
    // After a retransmit timeout, record the highest sequence number
    // transmitted in the variable "recover" and exit the Fast Recovery
    // procedure if applicable.

    // RFC 3571
    // Further, the new value of RecoveryPoint MUST be preserved and the
    // loss recovery algorithm outlined in this document MUST be
    // terminated.

    dupAcks = 0;  // Exit the Fast Recovery procedure
#ifdef TCP_SACK
    sendRecover = sendMax;

    // After a retransmit timeout the data sender SHOULD turn off all of
    // the SACKed bits [RFC 2018]
    sendHoles = 0;
#endif
    sendFack = sendUna;
    rxmitData = 0;
    sendAwin = 0;

    // Retransmit a packet
    int size = 14 + 60 + 60 + mss;  // XXX Assume MAC, IPv4, TCP
    Handle<InetMessenger> m = new InetMessenger(&InetReceiver::output, size, size);
    Handle<Address> addr;
    m->setLocal(addr = socket->getLocal());
    m->setRemote(addr = socket->getRemote());
    m->setLocalPort(socket->getLocalPort());
    m->setRemotePort(socket->getRemotePort());
    m->setType(IPPROTO_TCP);
    Visitor v(m);
    conduit->accept(&v, conduit->getB());
}

void StreamReceiver::
startAckTimer()
{
    if (ackTimer.isEnabled())
    {
        return;
    }
    Socket::alarm(&ackTimer, DACK_TIMEOUT);
}

void StreamReceiver::
stopAckTimer()
{
    Socket::cancel(&ackTimer);
}

// Handles delayed ACK timer (200 msec)
void StreamReceiver::
delayedAck()
{
    if (!socket)
    {
        return;
    }

    if (recvAcked < recvNext)
    {
        // Send ACK
        ackNow = true;
        int size = 14 + 60 + 60 + mss;  // XXX Assume MAC, IPv4, TCP
        Handle<InetMessenger> m = new InetMessenger(&InetReceiver::output, size, size);
        Handle<Address> addr;
        m->setLocal(addr = socket->getLocal());
        m->setRemote(addr = socket->getRemote());
        m->setLocalPort(socket->getLocalPort());
        m->setRemotePort(socket->getRemotePort());
        m->setType(IPPROTO_TCP);
        Visitor v(m);
        conduit->accept(&v, conduit->getB());
    }
}
