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

#include <algorithm>
#include "stream.h"

int StreamReceiver::
countOptionSize(u16 flag)
{
    int optlen = 0;
    if (flag & TCPHdr::SYN)
    {
        optlen += sizeof(TCPOptMss);
#ifdef TCP_SACK
        optlen += sizeof(TCPOptSackPermitted);
#endif  // TCP_SACK
    }
#ifdef TCP_SACK
    else if (sack && !(flag & TCPHdr::RST) && (flag & TCPHdr::ACK) && asb[0].data)
    {
        Ring::Vec* block;

        optlen += 2;
        for (block = &asb[TCPHdr::ASB_MAX - 1]; asb <= block; --block)
        {
            if (block->data)
            {
                break;
            }
        }
        optlen += (block + 1 - asb) * (2 * sizeof(s32));
    }
#endif  // TCP_SACK
    return (optlen + 3) & ~3;
}

int StreamReceiver::
fillOptions(u8* opt, u16 flag)
{
    u8* ptr = opt;
    if (flag & TCPHdr::SYN)
    {
        new(ptr) TCPOptMss(mss);
        ptr += sizeof(TCPOptMss);
#ifdef TCP_SACK
        new(ptr) TCPOptSackPermitted();
        ptr += sizeof(TCPOptSackPermitted);
#endif
    }
#ifdef TCP_SACK
    else if (sack && !(flag & TCPHdr::RST) && (flag & TCPHdr::ACK) && asb[0].data)
    {
        while (((ptr - opt) & 3) != 2)
        {
            new(ptr) TCPOptNop;
            ptr += sizeof(TCPOptNop);
        }
        TCPOptSack* optSack = new(ptr) TCPOptSack(recvRing.getHead(), recvRing.getSize(), asb, recvNext - recvRing.getUsed());
        ptr += optSack->len;
    }
#endif  // TCP_SACK

    while ((ptr - opt) & 3)
    {
        new(ptr) TCPOptEol;
        ptr += sizeof(TCPOptEol);
    }
    return ptr - opt;
}

// For SYN_RCVD and SYN_SEND
s32 StreamReceiver::
getSendableWithSyn(u16& flag)
{
    if (sendNext != iss)
    {
        // Do not send data only segment until connection is established.
        ASSERT(iss == sendUna);
        flag = 0;
        return 0;
    }
    flag = TCPHdr::SYN;
    return 1 + sendRing.getUsed();  // or one for just SYN
}

s32 StreamReceiver::
getSendable()
{
    // In persist state, exit the fast recovery
    if (sendWin == 0 && 0 < sendRing.getUsed())
    {
        if (!rxmitTimer.isEnabled())
        {
            onxt = sendNext = sendUna;  // To send a probe.
        }

        // Cancel RTT estimators. [Karn's algorithm]
        rttTiming = 0;

        // RFC 3782
        sendRecover = sendMax;
        dupAcks = 0;

        hole = 0;
        sendHoles = 0;
        sendFack = sendUna;
        rxmitData = 0;
        sendAwin = 0;

        fastRxmit = false;
    }

    if (RXMIT_THRESH <= dupAcks)
    {
#ifdef TCP_SACK
        if (sack)
        {
            //
            // (SACK-2) Override sendNext if any SACK-generated retransmissions.
            //
            hole = getSackHole();
            if (hole)
            {
                sendNext = hole->rxmit;
            }
        }
        else
#endif  // TCP_SACK
        {
            if (fastRxmit)
            {
                sendNext = sendUna;     // Retransmit the lost segment
            }
        }
    }

    s32 sendable = sendRing.getUsed() - (sendNext - sendUna);
#ifdef TCP_SACK
    // (SACK-2) Adjust sendable
    if (hole)
    {
        ASSERT(hole->rxmit == sendNext);
        sendable = std::min(sendable, hole->end - hole->rxmit);
    }
#endif  // TCP_SACK
    return sendable;
}

// For FIN_WAIT1, CLOSING, and LAST_ACK
s32 StreamReceiver::
getSendableWithFin(u16& flag)
{
    s32 sendable = getSendable();
    if (sendable < 0)
    {
        flag = 0;
        return 0;   // FIN has been sent and the retransmission timer has not expired yet.
    }
    if (!hole)
    {
        flag = TCPHdr::FIN;
        return sendable + 1;
    }
    return sendable;
}

// Detects silly window syndrome
bool StreamReceiver::
canSend(s32 len, s32 mss, u16 flag)
{
    //
    // Receiver silly window avoidance algorithm
    //
    long reduction = recvRing.getUnused() - recvWin;
    if (std::min(recvRing.getSize() / 2, 2L * mss) <= reduction ||
        mss <= reduction && recvWin < mss)
    {
        recvWin = recvRing.getUnused();
        return true;   // Send a window update
    }
    if (ackNow || (flag & (TCPHdr::RST | TCPHdr::FIN | TCPHdr::SYN)))
    {
        return true;    // Do not delay
    }

    //
    // The sender's SWS avoidance algorithm
    //
    if (len <= 0)
    {
        return false;
    }
    if (mss <= len)
    {
        return true;    // A maximum-sized segment can be sent.
    }
    bool acked;
    if (nagle)
    {
        acked = (sendNext == sendUna) ? true : false;
    }
    else
    {
        acked = true;
    }
    if (acked)
    {
        if (flag & TCPHdr::PSH)    // PSH is set if we have no more data to send.
        {
            return true;    // The data is pushed and all queued data can be sent now.
        }
        if (sendMaxWin / 2 <= len)
        {
            return true;    // At least a fraction of the maximum window can be sent.
        }
    }
    if (sendNext < sendMax)
    {
        return true;    // The override timeout occurs (retransmission)
    }
    if (fastRxmit)
    {
        return true;    // Bypass sender silly window avoidance for fast retransmit
    }

    return false;
}

bool StreamReceiver::
send(InetMessenger* m, s32 sendable, u16 flag)
{
    // If the window size is zero and the persist timer has been expired,
    // send a window probe.
    int win = std::min(sendWin, cWin);

    if (RXMIT_THRESH <= dupAcks && sack)
    {
#ifdef TCP_SACK
        //
        // (SACK-1) During SACK loss recovery period, cWin is checked later.
        //
        win = sendWin;
#endif  // TCP_SACK
    }
#ifdef TCP_LIMITED_TRANSMIT
    if (LIMITED_THRESH == dupAcks)
    {
        // the sender can only send two segments beyond the congestion window
        // (cwnd). [RFC 3042]
        win = sendWin;
    }
#endif  // TCP_LIMITED_TRANSMIT

    if (win == 0 && !rxmitTimer.isEnabled())
    {
        persist = true;
        if (!(flag & TCPHdr::SYN))
        {
            ackNow = true;      // To override Persist timer SWS
        }
        win = 1;
    }

    // Calculate useable size
    s32 useable = sendUna + win - sendNext;
#ifdef TCP_SACK
    if (sack)
    {
        //
        // (SACK-3) If cwnd - pipe >= 1 SMSS the sender SHOULD transmit one or more
        //          segments [RFC 3517]
        //
        if (RXMIT_THRESH <= dupAcks && !fastRxmit)
        {
            if (cWin - sendAwin < mss)
            {
                useable = 0;
            }
        }
    }
#endif  // TCP_SACK

#ifdef TCP_LIMITED_TRANSMIT
    if (LIMITED_THRESH == dupAcks)
    {
        // The amount of outstanding data would remain less than or equal
        // to the congestion window plus 2 segments. [RFC 3042]
        useable = std::min(useable, std::max(0, cWin + 2 * mss - sendAwin));
    }
#endif  // TCP_LIMITED_TRANSMIT

    // Calculate send length
    int optlen = countOptionSize(flag);
    s32 len = std::min(mss - optlen, std::min(sendable, useable));
    if (0 < len && len == sendable && !hole &&
        (flag & (TCPHdr::SYN | TCPHdr::RST | TCPHdr::FIN)) == 0)
    {
        flag |= TCPHdr::PSH;    // No more data to send:
    }
    if ((flag & TCPHdr::FIN) && len < sendable)
    {
        flag &= ~TCPHdr::FIN;   // More data to send:
    }
    if (len <= 0 && flag == 0 && !ackNow)
    {
        return false;
    }

    // Perform Silly Window Syndrome avoidance
    if (!canSend(len, mss - optlen, flag))
    {
        // Wait for override timer time-out
        if (sendNext < onxt)
        {
            sendNext = onxt;
        }
        if (0 < len)
        {
            // Set persist/SWS override timer. Note SWS avoidance does not
            // apply for packet retransmission.
            startRxmitTimer();
        }
        if (recvAcked < recvNext)
        {
            // Delayed ACK
            // 4.2 Generating Acknowledgments [RFC 2581]
            // 4.2.3.2  When to Send an ACK Segment [RFC 1122]
            startAckTimer();
        }
        return false;
    }

    // Make data portion
    if (0 < len)
    {
        long count = len;
        if (flag & TCPHdr::SYN)
        {
            --count;
        }
        if (flag & TCPHdr::FIN)
        {
            --count;
        }
        if (0 < count)
        {
            m->movePosition(-count);
            sendRing.peek(m->fix(count), count, sendNext - sendUna);
        }
    }

    // Make TCP header
    m->movePosition(-(sizeof(TCPHdr) + optlen));
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr) + optlen));
    if (state != &stateSynReceived)
    {
        recvAcked = recvNext;
    }
    else
    {
        recvAcked = irs + 1;  // Just for SYN
    }
    tcphdr->ack = htonl(recvAcked);
    tcphdr->src = htons(m->getLocalPort());
    tcphdr->dst = htons(m->getRemotePort());
    ASSERT(tcphdr->src != 0);
    ASSERT(tcphdr->dst != 0);
    TCPSeq seq = (0 < len) ? sendNext : sendMax;
    tcphdr->seq = htonl(seq);
    if (seq < sendUp)
    {
        // Set urgent offset
        flag |= TCPHdr::URG;
        s32 offset = sendUp - seq;
        u16 urg = (u16) ((65495 < offset) ? 65535 : offset);
#ifdef TCP_STD_URG
        --urg;
#endif  // TCP_STD_URG
        tcphdr->urg = htons(urg);
    }
    else
    {
        tcphdr->urg = 0;
        sendUp = sendUna;
    }
    if (state != &stateSynSent)
    {
        flag |= TCPHdr::ACK;
    }
    ackNow = false;
    stopAckTimer();
    ASSERT((flag & 0x0fc0) == 0);   // RFC 793 only
    tcphdr->flag = htons(flag);
    tcphdr->win = htons(recvWin);
    tcphdr->sum = 0;
    tcphdr->setHdrSize(sizeof(TCPHdr) + optlen);

    // Make TCP option(s)
    fillOptions(reinterpret_cast<u8*>(tcphdr) + sizeof(TCPHdr), flag);

    sendNext += len;
    if (sendMax < sendNext) // Not a retransmission?
    {
        // In case sending a window probe, sendNext is out of window and
        // must not be reflected to sendMax.
        if (sendNext <= TCPSeq(sendUna + sendWin))
        {
            // ++interface->tcpStat.outSegs;
            sendMax = sendNext;
        }
        else
        {
            // ++interface->tcpStat.retransSegs;
        }

        // If round trip timer isn't running, start it
        if (rttTiming != 0)
        {
            rttTiming = DateTime::getNow();
            rttSeq = seq;
        }
    }
    else if (0 < len)
    {
        // ++interface->tcpStat.retransSegs;
    }

    // Start retransmission timer
    if (sendNext != sendUna)  // Not for an ACK-only segment
    {
        startRxmitTimer();
    }

#ifdef TCP_SACK
    if (hole)
    {
        //
        // (SACK-5) Update scoreboard rxmit pointer and rxmitData size.
        //
        hole->rxmit += len;
        rxmitData += len;
    }
#endif  // TCP_SACK

    //
    // (SACK-6) Update aWin
    //
    // Update sendAwin to reflect the new data that was sent.
    sendAwin = (sendMax - sendFack) + rxmitData;

    //
    // (SACK-7) Turn off fastRxmit
    //
    fastRxmit = false;

#ifdef TCP_SACK
    //
    // (SACK-8) Restore sendNext
    //
    if (sendNext < onxt)
    {
        sendNext = onxt;
    }
#endif

    lastSend = DateTime::getNow();  // For re-starting the idle connection.

    return true;
}

bool StreamReceiver::
StateSynSent::output(InetMessenger* m, StreamReceiver* s)
{
    u16 flag;
    s32 count = s->getSendableWithSyn(flag);
    return s->send(m, count, flag);
}

bool StreamReceiver::
StateSynReceived::output(InetMessenger* m, StreamReceiver* s)
{
    u16 flag;
    s32 count = s->getSendableWithSyn(flag);
    return s->send(m, count, flag);
}

bool StreamReceiver::
StateEstablished::output(InetMessenger* m, StreamReceiver* s)
{
    s32 count = s->getSendable();
    return s->send(m, count, 0);
}

bool StreamReceiver::
StateFinWait1::output(InetMessenger* m, StreamReceiver* s)
{
    u16 flag;
    s32 count = s->getSendableWithFin(flag);
    return s->send(m, count, flag);
}

bool StreamReceiver::
StateFinWait2::output(InetMessenger* m, StreamReceiver* s)
{
    s32 count = s->getSendable();
    return s->send(m, count, 0);
}

bool StreamReceiver::
StateCloseWait::output(InetMessenger* m, StreamReceiver* s)
{
    s32 count = s->getSendable();
    return s->send(m, count, 0);
}

bool StreamReceiver::
StateLastAck::output(InetMessenger* m, StreamReceiver* s)
{
    u16 flag;
    s32 count = s->getSendableWithFin(flag);
    return s->send(m, count, flag);
}

bool StreamReceiver::
StateClosing::output(InetMessenger* m, StreamReceiver* s)
{
    u16 flag;
    s32 count = s->getSendableWithFin(flag);
    return s->send(m, count, flag);
}

bool StreamReceiver::
StateTimeWait::output(InetMessenger* m, StreamReceiver* s)
{
    u16 flag;
    s32 count = s->getSendableWithFin(flag);
    return s->send(m, count, flag);
}
