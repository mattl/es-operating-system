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

#include <errno.h>
#include <algorithm>
#include <es/handle.h>
#include "stream.h"

void StreamReceiver::
sendReset(InetMessenger* m)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    long offset = tcphdr->getHdrSize();
    long len = getSegmentLength(m, offset, flag);

    if (flag & TCPHdr::RST)
    {
        return;
    }

    int size = 14 + 60 + sizeof(TCPHdr);  // XXX Assume MAC, IPv4
    Handle<InetMessenger> rst = new InetMessenger(&InetReceiver::output, size, size - sizeof(TCPHdr));

    tcphdr = static_cast<TCPHdr*>(rst->fix(sizeof(TCPHdr)));
    tcphdr->src = htons(m->getLocalPort());
    tcphdr->dst = htons(m->getRemotePort());
    if (!(flag & TCPHdr::ACK))
    {
        tcphdr->seq = 0;
        tcphdr->ack = htonl(seq + len);
        tcphdr->flag = htons(TCPHdr::RST | TCPHdr::ACK);
    }
    else
    {
        tcphdr->seq = htonl(ack);
        tcphdr->ack = 0;
        tcphdr->flag = htons(TCPHdr::RST);
    }
    tcphdr->win = 0;
    tcphdr->sum = 0;
    tcphdr->urg = 0;
    tcphdr->setHdrSize(sizeof(TCPHdr));

    Handle<Address> addr;
    rst->setLocal(addr = m->getLocal());
    rst->setRemote(addr = m->getRemote());
    rst->setLocalPort(m->getLocalPort());
    rst->setRemotePort(m->getRemotePort());
    rst->setType(IPPROTO_TCP);
    Visitor v(rst);
    conduit->accept(&v, conduit->getB());
}

// Send a reset segment: <SEQ=SND.NXT><CTL=RST>
void StreamReceiver::
sendReset()
{
    int size = 14 + 60 + sizeof(TCPHdr);  // XXX Assume MAC, IPv4
    Handle<InetMessenger> rst = new InetMessenger(&InetReceiver::output, size, size - sizeof(TCPHdr));

    TCPHdr* tcphdr = static_cast<TCPHdr*>(rst->fix(sizeof(TCPHdr)));
    tcphdr->src = htons(socket->getLocalPort());
    tcphdr->dst = htons(socket->getRemotePort());
    tcphdr->seq = htonl(sendMax);
    tcphdr->ack = 0;
    tcphdr->flag = htons(TCPHdr::RST);
    tcphdr->win = 0;
    tcphdr->sum = 0;
    tcphdr->urg = 0;
    tcphdr->setHdrSize(sizeof(TCPHdr));

    rst->setLocal(Handle<Address>(socket->getLocal()));
    rst->setRemote(Handle<Address>(socket->getRemote()));
    rst->setLocalPort(socket->getLocalPort());
    rst->setRemotePort(socket->getRemotePort());
    rst->setType(IPPROTO_TCP);
    Visitor v(rst);
    conduit->accept(&v, conduit->getB());
}

bool StreamReceiver::
trim(u16& flag, TCPSeq& seq, u16& urg, long& len, long& offset)
{
    bool ackOnly = (len <= 0) ? true : false;

    // Left edge (duplicated bytes)
    s32 drop = recvNext - seq;
    if (0 < drop)
    {
        if (flag & TCPHdr::SYN)
        {
            flag &= ~TCPHdr::SYN;
            ++seq;
            --len;
            if (1 < urg)
            {
                --urg;
            }
            else
            {
                flag &= ~TCPHdr::URG;
            }
            --drop;
        }

        if (len <= drop)
        {
            if (flag & TCPHdr::FIN)
            {
                flag &= ~TCPHdr::FIN;
            }
            drop = len;
        }

        if (0 < drop)
        {
            seq += drop;
            len -= drop;
            offset += drop;
            if (drop < urg)
            {
                urg -= drop;
            }
            else
            {
                flag &= ~TCPHdr::URG;
                urg = 0;
            }
        }
    }

    // Right edge
    drop = (seq + len) - (recvNext + recvWin);
    if (0 < drop)
    {
        if (flag & TCPHdr::FIN)
        {
            flag &= ~TCPHdr::FIN;
            --len;
            --drop;
        }
        if (len < drop)
        {
            drop = len;
        }
        len -= drop;
    }

    if (0 < len)
    {
        ASSERT(0 < recvWin);
        ASSERT(recvNext <= seq &&
               seq < TCPSeq(recvNext + recvWin) ||
               recvNext <= TCPSeq(seq + len - 1) &&
               TCPSeq(seq + len - 1) < TCPSeq(recvNext + recvWin));
        return true;
    }
    ASSERT(len == 0);
    if (0 == recvWin && seq == recvNext ||
        0 < recvWin && recvNext <= seq && seq < TCPSeq(recvNext + recvWin))
    {
        return true;
    }

    ackNow = !ackOnly;
    return false;
}

// The combined slow-start with congestion avoidance algorithm
//
// The slow start is used when cWin (congestion window) < ssThresh
// (slow start threshold), while the congestion avoidance is used
// when cWin >= ssThresh.
void StreamReceiver::
openWindow(TCPSeq ack)
{
    if (RXMIT_THRESH <= dupAcks)
    {
#ifdef TCP_SACK
        if (sack)
        {
            // During loss recovery phase, cWin is held constant.
        }
        else
#endif
        {
            // Deflate cWin by the amount of new data acknowledged.
            cWin -= (ack - sendUna);
            cAcked += (ack - sendUna);

            // If the partial ACK acknowledges at least one SMSS of
            // new data, then add back SMSS bytes to cWin.
            while (mss <= cAcked)
            {
                cAcked -= mss;
                cWin += mss;
            }
        }
        return;
    }

    if (cWin < ssThresh)
    {
        // Slow start (open exponentially): increase cWin by at
        // most one segment on each ack for new data.
        cWin += std::min(ack - sendUna, mss);
        if (ssThresh <= cWin)
        {
            // Switching to congestion avoidance
            cAcked = 0;
        }
    }
    else
    {
        // Congestion avoidance (open linearly): increase cWin by
        // 1 full-sized segment per RTT on each non-duplicate ACK.
        cAcked += (ack - sendUna);
        while (cWin <= cAcked)
        {
            cAcked -= cWin;
            cWin += mss;
        }
    }

    // Check expand overflow.
    if (65535 < cWin) // [MAY] support window scale option
    {
        cWin = 65535;
    }
}

// return true if FIN is acked
bool StreamReceiver::
ack(TCPSeq seq, TCPSeq ack, s32 win, s32 sent, long len)
{
    if (ack <= sendUna)
    {
        //
        // Duplicated/Old ACK. Duplicated ACK can be ignored. [RFC 793]
        //
        if (dupAcks < RXMIT_THRESH)
        {
            if (sendUna == sendMax) // Nothing to send
            {
                dupAcks = 0;
                return true;
            }
            if (win != sendWin ||   // Window update
                len != 0 ||         // Not an ACK only segment
                ack != sendUna)     // out-of-order ACK
            {
                return true;
            }
        }

        ++dupAcks;
        if (RXMIT_THRESH < dupAcks)
        {
#ifdef TCP_SACK
            if (sack)
            {
                ASSERT(sendAwin == sendNext - sendFack + rxmitData);
            }
            else
#endif // TCP_SACK
            {
                cWin += mss;
            }
        }
        else if (dupAcks == RXMIT_THRESH ||
                 (sack && RXMIT_THRESH * mss < sendFack - sendUna))
        {
            ASSERT(dupAcks <= RXMIT_THRESH);  // in case of SACK
            // If ack doesn't cover more than sendRecover, then do not
            // enter the Fast Retransmit and Fast Recovery procedure. [RFC 3728]
            //
            // Note in NewReno sendRecover is pulled along with (ack - 1).
            // This test prevents the sender to reenter the fast recover after
            // a retransmission timeout.
            if (ack <= TCPSeq(sendRecover - 1))
            {
                dupAcks = 0;
                return true;
            }

            cutThresh();
            sendRecover = sendMax;

            // Retransmit the lost segment
            fastRxmit = true;

            // Cancel RTT estimators. [Karn's algorithm]
            rttTiming = 0;

            resetRxmitTimer();

#ifdef TCP_SACK
            if (sack)
            {
                dupAcks = RXMIT_THRESH; // in case dupAcks < RXMIT_THRESH
                cWin = ssThresh;        // ssthresh = cwnd = (FlightSize / 2) [RFC 3517]
            }
            else
#endif // TCP_SACK
            {
                cWin = ssThresh + 3 * mss;
                cAcked = 0;   // Count # of acked octets during fast recovery.
            }
        }
#ifdef TCP_LIMITED_TRANSMIT
        else if (dupAcks == LIMITED_THRESH ||
                 (sack || LIMITED_THRESH * mss < sendFack - sendUna))
        {
            if (ack <= TCPSeq(sendRecover - 1))
            {
                // Don't invoke the limited transmit algorithm after RTO.
                dupAcks = 0;
                return true;
            }
            dupAcks = LIMITED_THRESH;   // in case dupAcks < LIMITED_THRESH
        }
#endif // TCP_LIMITED_TRANSMIT
        return true;
    }

    //
    // Non-duplicated ACK
    //
    ASSERT(sendUna < ack);
    if (RXMIT_THRESH <= dupAcks)
    {
        // Cancel RTT estimators. [Karn's algorithm]
        rttTiming = 0;

        if (ack < sendRecover)
        {
            // Partial acknowledgements:

            fastRxmit = true;   // Retransmit the lost segment
            if (dupAcks++ == RXMIT_THRESH)
            {
                // Impatient variants of NewReno
                resetRxmitTimer();
            }
            openWindow(ack);
        }
        else
        {
            // Full acknowledgements: Out of fast recovery.
#ifdef TCP_SACK
            if (sack)
            {
                cWin = std::min(ssThresh, sendAwin + mss);
            }
            else
#endif  // TCP_SACK
            {
                // Set cWin to std::min(ssThresh, FlightSize + SMSS) [RFC 3782]
                cWin = std::min(ssThresh, (sendMax - ack) + mss);
                // XXX should take measures to avoid a possible burst of data
            }
            dupAcks = 0;
        }
    }
    else
    {
        dupAcks = 0;
        resetRxmitTimer();
        openWindow(ack);
    }

    sendUna = ack;
    // When not in Fast Recovery, the value of the state variable "recover"
    // should be pulled along with the value of the state variable for
    // acknowledgments (typically, "snd_una") [RFC 3728]
    if (sendRecover < sendUna)
    {
        sendRecover = sendUna;
    }
    if (sendNext < sendUna)
    {
        sendNext = sendUna;
    }

    // As sendAwin is used for the limited transmit algorithm as well as SACK
    // loss recorvery, update sendFack and sendAwin no matter what.
    if (sendFack < sendUna)
    {
        sendFack = sendUna;
        sendAwin = sendNext - sendFack + rxmitData;
    }

    // Update the send buffer (sendPtr and sendLen)
    bool finAcked;
    s32 sendLen = sendRing.getUsed();
    if (sendLen < sent)
    {
        ASSERT(sendMax <= ack);
        --sent;    // for FIN
        finAcked = true;
    }
    else
    {
        finAcked = false;
    }
    if (0 < sent && 0 < sendLen)
    {
        s32 len = std::min(sent, sendLen);
        sendRing.skip(len);
        sendLen -= len;
        sent -= len;

        notify();
    }

    // Update RTT estimators
    if (rttTiming != 0 && rttSeq < ack)
    {
        updateRto(DateTime::getNow() - rttTiming);
    }

    // Update the send window
    if (sendWL1 < seq ||
        sendWL1 == seq && sendWL2 <= ack ||
        sendWL2 == ack && sendWin < win)        // Persist timer will be stopped
    {
        sendWin = win;
        sendWL1 = seq;
        sendWL2 = ack;
        if (sendMaxWin < sendWin)
        {
            sendMaxWin = sendWin;
        }
    }

    // Check persist state
    if (sendWin == 0 && 0 < sendRing.getUsed())
    {
        // Entered persist state. To send window probes, reset rxmit timer.
        persist = true;
        r0 = DateTime::getNow();    // So as not to timeout.
        resetRxmitTimer();
    }
    else
    {
        persist = false;
    }

    if (sendUna == sendMax)         // All outstanding data is acked
    {
        stopRxmitTimer();
    }

    return finAcked;
}

void StreamReceiver::
urg(InetMessenger* m, TCPSeq seq, u16 urg, long len)
{
    if (0 < urg)
    {
        TCPSeq up = seq + urg;
        recvUp = std::max(recvUp, up);
        hadUrg = false;
        haveUrg = (urg <= len) ? true : false;
    }
}

// Copy data into recvRing
bool StreamReceiver::
text(InetMessenger* m, u16& flag, TCPSeq seq,
     long len, long offset)
{
    s32 adv = len;
    if (flag & TCPHdr::FIN)
    {
        --adv;
    }

    // [RFC 1122 SHOULD / RFC 2525 to fix] Send RST to indicate data lost
    // Probe segments should be ignored if the window can
    // never subsequently increase. [RFC 2525]
    if (isShutdownInput() && 0 < adv)
    {
        err = ECONNABORTED;
        sendReset(m);
        abort();
        return false;
    }

    if (recvNext == seq)
    {
        if (0 < adv)    // Copy-in
        {
            if (flag & TCPHdr::PSH)
            {
                // Trigger sending an acknowledgement [RFC 813]
                ackNow = true;
            } // Otherwise, postpone sending an ACK
            recvRing.write(m->fix(adv, m->getPosition() + offset), adv, 0,
                           asb, TCPHdr::ASB_MAX);

            // Do not shrink window right edge
            recvNext += adv;
            recvWin -= adv;

            notify();
        }
    }
    else    // Out-of-order:
    {
        // A TCP receiver SHOULD send an immediate duplicate ACK
        // when an out-of-order segment arrives. [RFC 2581 - 3.2,
        // obsoletes RFC 1122 4.2.2.21]
        ASSERT(recvNext < seq);

        if (0 < adv)
        {
            ackNow = true;
        }

        // [RFC 1122] While it is not strictly required,
        // a TCP SHOULD be capable of queuing out-of-order
        // TCP segments.
        recvRing.write(m->fix(adv, m->getPosition() + offset), adv, seq - recvNext,
                       asb, TCPHdr::ASB_MAX);

        // Trun off FIN
        if (flag & TCPHdr::FIN)
        {
            flag &= ~TCPHdr::FIN;
            --len;
        }
    }

    return true;
}

bool StreamReceiver::
option(TCPHdr* tcphdr)
{
    u16 flag = ntohs(tcphdr->flag);
    int mss = getDefaultMSS();

    int optlen = tcphdr->getHdrSize() - sizeof(TCPHdr);
    u8* opt = reinterpret_cast<u8*>(tcphdr) + sizeof(TCPHdr);   // XXX use m->fix
    while (0 < optlen && *opt != TCPHdr::OPT_EOL)
    {
        int len;
        if (*opt == TCPHdr::OPT_NOP)
        {
            len = 1;
        }
        else if (optlen < 2)
        {
            return false;
        }
        else
        {
            len = opt[1];
            if (len < 2 || optlen < len)
            {
                return false;
            }
        }

        switch (*opt)
        {
          case TCPHdr::OPT_MSS:
            // Initialize congestion window to one segment. It will be increased
            // by on segment each time an ACK is received.
            if (len != sizeof(TCPOptMss) || !(flag & TCPHdr::SYN))
            {
                return false;
            }
            mss = reinterpret_cast<TCPOptMss*>(opt)->getMSS();
            if (mss <= 0)
            {
                return false;
            }
            break;
#ifdef TCP_SACK
          case TCPHdr::OPT_SACKP:
            if (len != sizeof(TCPOptSackPermitted) || !(flag & TCPHdr::SYN))
            {
                return false;
            }
            sack = true;
            break;
          case TCPHdr::OPT_SACK:
            if ((len - 2) & 7)
            {
                return false;
            }
            updateScoreboard(ntohl(tcphdr->ack), reinterpret_cast<TCPOptSack*>(opt));
            break;
#endif // TCP_SACK
          default:
            // Do not process unknown options.
            break;
        }

        opt += len;
        optlen -= len;
    }

    if (optlen < 0)
    {
        return false;
    }

    if (flag & TCPHdr::SYN)
    {
        // Update mss to the default minimum value (536), if
        // TCPHdr::OPT_MSS option is not specified.
        this->mss = std::min(this->mss, mss);
        cWin = getInitialCongestionWindowSize();
        ssThresh = DEF_SSTHRESH;
    }

    return true;
}

bool StreamReceiver::
StateClosed::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);

    if (!(flag & TCPHdr::RST))
    {
        s->sendReset(m);
    }
    return false;
}

bool StreamReceiver::
StateListen::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    u16    urg = ntohs(tcphdr->urg);
    long   offset = tcphdr->getHdrSize();
    long   len = s->getSegmentLength(m, offset, flag);

    if (flag & TCPHdr::RST)
    {
        return false;   // Ignore RST
    }

    if (flag & TCPHdr::ACK)
    {
        s->sendReset(m);
        return false;
    }

    if (!(flag & TCPHdr::SYN))
    {
        // Unlikely to get here, but drop the segment and return
        return false;
    }

    Handle<Address> local = m->getLocal();

    // Clone new socket
    ASSERT(s->socket);
    Socket* socket = new Socket(s->socket->getAddressFamily(), es::Socket::Stream);
    socket->setLocal(local);
    socket->setLocalPort(m->getLocalPort());
    socket->setRemote(Handle<Address>(m->getRemote()));
    socket->setRemotePort(m->getRemotePort());

    SocketInstaller installer(socket);
    socket->getProtocol()->accept(&installer);

    StreamReceiver* accepted = dynamic_cast<StreamReceiver*>(socket->getReceiver());
    ASSERT(accepted);

    // Initialize ISS and sequence number variables
    accepted->mss = accepted->getDefaultMSS(local->getPathMTU());
    accepted->cWin = accepted->getInitialCongestionWindowSize();
    accepted->ssThresh = DEF_SSTHRESH;
    accepted->iss = accepted->isn(m);
    accepted->recvAcked = accepted->irs = accepted->irs = seq;
    accepted->recvNext = seq + 1;       // just for SYN. Segment will be trimmed later.
    accepted->recvUp = accepted->recvNext;

    accepted->sendUna = accepted->iss;
    accepted->sendNext = accepted->iss; // Updated (i.e. +1 for SYN) later by TCPOutput().
    accepted->sendMax = accepted->iss;  // Updated (i.e. +1 for SYN) later by TCPOutput().
    accepted->sendUp = accepted->iss;

    accepted->lastSack = accepted->sendFack = accepted->sendRecover = accepted->sendUna;
    accepted->rxmitData = 0;
    accepted->sendAwin = 0;

    if (!accepted->option(tcphdr))
    {
        accepted->abort();
        return false;
    }

    accepted->listening = s;

    if (s->parConn + s->pendingConn >= s->backLogCount)
    {
        accepted->abort();
        return false; // Ignore SYN when full
    }
    else
    {
        s->parConn++; // count partial connection
    }
    //++interface->tcpStat.passiveOpens;
    accepted->setState(stateSynReceived);

    // To send <SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK>,
    // move on to URG bit check in TCPIn()
    Visitor v(m);
    accepted->conduit->accept(&v);

    return false;
}

bool StreamReceiver::
StateSynSent::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    u16    urg = ntohs(tcphdr->urg);
    long   offset = tcphdr->getHdrSize();
    long   len = s->getSegmentLength(m, offset, flag);

    if ((flag & TCPHdr::ACK) && (ack <= s->iss || s->sendMax < ack))
    {
        s->sendReset(m);
        return false;
    }

    if (flag & TCPHdr::RST)
    {
        if (flag & TCPHdr::ACK)
        {
            s->err = ECONNRESET;
            s->abort();
        }
        return false;
    }

    if (!(flag & TCPHdr::SYN))
    {
        return false;
    }

    // Initialize receive sequence numbers
    s->recvUp = s->recvNext = seq + 1;  // just for SYN. Segment will be trimmed later.
    s->recvAcked = s->irs = seq;

    // Process TCP options (Get 'sendMss' if available)
    if (!s->option(tcphdr))
    {
        return false;
    }

    if ((flag & TCPHdr::ACK) && s->iss < ack)   // SYN has been ACKed
    {
        // Update RTT estimators
        if (s->rttTiming != 0 && s->rttSeq < ack)
        {
            s->updateRto(DateTime::getNow() - s->rttTiming);
        }

        ++s->sendUna;   // Accept just SYN for now
        if (s->sendNext < s->sendUna)
        {
            s->sendNext = s->sendUna;
        }

        s->setState(stateEstablished);

        // Update the send window (sendWin) [RFC1122]
        s->sendWin = ntohs(tcphdr->win);
        s->sendWL1 = seq;
        s->sendWL2 = ack;
        s->sendMaxWin = s->sendWin;

        // ++interface->tcpStat.currEstab;
        s->notify();

        s->ackNow = true;   // XXX
    }
    else
    {
        s->setState(stateSynReceived);
        s->sendNext = s->sendUna;       // Then send SYN,ACK
    }

    if (!s->trim(flag, seq, urg, len, offset))
    {
        return true;
    }

    if (!(flag & TCPHdr::ACK))
    {
        return false;
    }

    if (s->sendMax < ack)
    {
        // The ACK acks something not yet sent.
        s->ackNow = true;
        return true;
    }
    s->ack(seq, ack, ntohs(tcphdr->win), ack - s->sendUna, len);

    if (flag & TCPHdr::URG)
    {
        s->urg(m, seq, urg, len);
    }

    if (!s->text(m, flag, seq, len, offset))
    {
        return false;
    }

    if (flag & TCPHdr::FIN)
    {
        ++s->recvNext;
        s->ackNow = true;
        s->setState(stateCloseWait);
    }

    return true;
}

bool StreamReceiver::
StateSynReceived::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    u16    urg = ntohs(tcphdr->urg);
    long   offset = tcphdr->getHdrSize();
    long   len = s->getSegmentLength(m, offset, flag);

    // In the SYN-RECEIVED state, and if the incoming segment
    // carries an unacceptable ACK, send a reset. cf. [RFC 793 page 36]
    // Performing the ACK field test prior to any other check is
    // required for the defense against the LAND attack. Otherwise,
    // the segment is likely to be acknowledged (not reset) by
    // the sequence number check failure.
    //
    // In RFC 793, it says:
    //
    //   If SND.UNA =< SEG.ACK =< SND.NXT then enter ESTABLISHED state
    //   and continue processing.
    //
    // However, it appears not so correct. A correct
    // way would be "SND.UNA < SEG.ACK <= SND.NXT". Otherwise, an ACK
    // for not our SYN would be accepted.
    //
    // In RFC 764, it says:
    //
    //   If the RST bit is off and SND.UNA < SEG.ACK =< SND.NXT then set
    //   SND.UNA <- SEG.ACK, remove any acknowledged segments from the
    //   retransmission queue, and enter ESTABLISHED state.
    //
    // So is it a typo in RFC 793?
    if ((flag & TCPHdr::ACK) && (ack < s->sendUna || s->sendMax < ack))
    {
        s->sendReset(m);
        return false;
    }

    // Yet another defence for the LAND attack
    if (seq < s->irs)
    {
        s->sendReset(m);
        return false;
    }

    if (flag & TCPHdr::RST)
    {
        s->reset(seq, ECONNREFUSED);
        return false;
    }

    if (!s->trim(flag, seq, urg, len, offset))
    {
        return true;
    }

    if (flag & TCPHdr::SYN)
    {
        s->sendReset(m);
        s->abort();
        return false;
    }

    if (flag & TCPHdr::ACK)
    {
        s32 sent = ack - s->sendUna;
        if (s->iss < ack)   // SYN has been ACKed
        {
            // Note ack has been verified before the segment is trimmed.
            if (s->sendUna == s->iss)
            {
                ASSERT(s->sendUna < ack);
                --sent;    // for SYN
            }

            if (!s->isShutdownOutput())
            {
                s->setState(stateEstablished);
                // ++interface->tcpStat.currEstab;
            }
            else
            {
                // close() has been called.
                s->setState(stateFinWait1);
            }

            // RFC1122
            s->sendWin = ntohs(tcphdr->win);
            s->sendWL1 = seq;
            s->sendWL2 = ack;
            s->sendMaxWin = s->sendWin;

            if (s->listening)
            {
                StreamReceiver* listening = s->listening;
                Synchronized<es::Monitor*> method(listening->monitor);

                listening->parConn--; 
                ASSERT(listening->parConn >= 0);
                listening->pendingConn++;
                listening->accepted.addLast(s);
                listening->notify();
            }
        }

        if (!s->option(tcphdr))
        {
            return false;
        }
        s->ack(seq, ack, ntohs(tcphdr->win), sent, len);
    }

    if (flag & TCPHdr::URG)
    {
        s->urg(m, seq, urg, len);
    }

    if (!s->text(m, flag, seq, len, offset))
    {
        return false;
    }

    if (flag & TCPHdr::FIN)
    {
        ++s->recvNext;
        s->ackNow = true;
        s->setState(stateCloseWait);
    }

    return true;
}

bool StreamReceiver::
StateEstablished::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    u16    urg = ntohs(tcphdr->urg);
    long   offset = tcphdr->getHdrSize();
    long   len = s->getSegmentLength(m, offset, flag);

    if (flag & TCPHdr::RST)
    {
        s->reset(seq, ECONNRESET);
        return false;
    }

    if (!s->trim(flag, seq, urg, len, offset))
    {
        return true;
    }

    if (flag & TCPHdr::SYN)
    {
        s->sendReset(m);
        s->abort();
        return false;
    }

    if (!(flag & TCPHdr::ACK))
    {
        return false;
    }

    if (s->sendMax < ack)
    {
        // The ACK acks something not yet sent.
        s->ackNow = true;
        return true;
    }
    if (!s->option(tcphdr))
    {
        return false;
    }
    s->ack(seq, ack, ntohs(tcphdr->win), ack - s->sendUna, len);

    if (flag & TCPHdr::URG)
    {
        s->urg(m, seq, urg, len);
    }

    if (!s->text(m, flag, seq, len, offset))
    {
        return false;
    }

    if (flag & TCPHdr::FIN)
    {
        ++s->recvNext;
        s->ackNow = true;
        s->setState(stateCloseWait);
    }

    return true;
}

bool StreamReceiver::
StateFinWait1::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    u16    urg = ntohs(tcphdr->urg);
    long   offset = tcphdr->getHdrSize();
    long   len = s->getSegmentLength(m, offset, flag);

    if (flag & TCPHdr::RST)
    {
        s->reset(seq, ECONNRESET);
        return false;
    }

    if (!s->trim(flag, seq, urg, len, offset))
    {
        return true;
    }

    if (flag & TCPHdr::SYN)
    {
        s->sendReset(m);
        s->abort();
        return false;
    }

    if (!(flag & TCPHdr::ACK))
    {
        return false;
    }

    if (s->sendMax < ack)
    {
        // The ACK acks something not yet sent.
        s->ackNow = true;
        return true;
    }
    if (!s->option(tcphdr))
    {
        return false;
    }
    if (s->ack(seq, ack, ntohs(tcphdr->win), ack - s->sendUna, len))
    {
        // FIN acked
        s->setState(stateFinWait2);
    }

    if (flag & TCPHdr::URG)
    {
        s->urg(m, seq, urg, len);
    }

    if (!s->text(m, flag, seq, len, offset))
    {
        return false;
    }

    if (flag & TCPHdr::FIN)
    {
        // Note if FIN has been acked, TCP has been switched to stateFinWait2.
        ++s->recvNext;
        s->ackNow = true;
        if (s->getState() == &stateFinWait1)
        {
            s->setState(stateClosing);
        }
        else
        {
            s->setState(stateTimeWait);
        }
    }

    return true;
}

bool StreamReceiver::
StateFinWait2::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    u16    urg = ntohs(tcphdr->urg);
    long   offset = tcphdr->getHdrSize();
    long   len = s->getSegmentLength(m, offset, flag);

    if (flag & TCPHdr::RST)
    {
        s->reset(seq, ECONNRESET);
        return false;
    }

    if (!s->trim(flag, seq, urg, len, offset))
    {
        return true;
    }

    if (flag & TCPHdr::SYN)
    {
        s->sendReset(m);
        s->abort();
        return false;
    }

    if (!(flag & TCPHdr::ACK))
    {
        return false;
    }

    if (s->sendMax < ack)
    {
        // The ACK acks something not yet sent.
        s->ackNow = true;
        return true;
    }
    if (!s->option(tcphdr))
    {
        return false;
    }
    s->ack(seq, ack, ntohs(tcphdr->win), ack - s->sendUna, len);

    if (flag & TCPHdr::URG)
    {
        s->urg(m, seq, urg, len);
    }

    if (!s->text(m, flag, seq, len, offset))
    {
        return false;
    }

    if (flag & TCPHdr::FIN)
    {
        ++s->recvNext;
        s->ackNow = true;
        s->setState(stateTimeWait);
    }

    return true;
}

bool StreamReceiver::
StateCloseWait::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    u16    urg = ntohs(tcphdr->urg);
    long   offset = tcphdr->getHdrSize();
    long   len = s->getSegmentLength(m, offset, flag);

    if (flag & TCPHdr::RST)
    {
        s->reset(seq, ECONNRESET);
        return false;
    }

    if (!s->trim(flag, seq, urg, len, offset))
    {
        return true;
    }

    if (flag & TCPHdr::SYN)
    {
        s->sendReset(m);
        s->abort();
        return false;
    }

    if (!(flag & TCPHdr::ACK))
    {
        return false;
    }

    if (s->sendMax < ack)
    {
        // The ACK acks something not yet sent.
        s->ackNow = true;
        return true;
    }
    if (!s->option(tcphdr))
    {
        return false;
    }
    s->ack(seq, ack, ntohs(tcphdr->win), ack - s->sendUna, len);

    return true;
}

bool StreamReceiver::
StateLastAck::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    u16    urg = ntohs(tcphdr->urg);
    long   offset = tcphdr->getHdrSize();
    long   len = s->getSegmentLength(m, offset, flag);

    if (flag & TCPHdr::RST)
    {
        s->reset(seq);
        return false;
    }

    if (!s->trim(flag, seq, urg, len, offset))
    {
        return true;
    }

    if (flag & TCPHdr::SYN)
    {
        s->sendReset(m);
        s->abort();
        return false;
    }

    if (!(flag & TCPHdr::ACK))
    {
        return false;
    }

    if (s->sendMax < ack)
    {
        // The ACK acks something not yet sent.
        s->ackNow = true;
        return true;
    }
    if (!s->option(tcphdr))
    {
        return false;
    }
    if (s->ack(seq, ack, ntohs(tcphdr->win), ack - s->sendUna, len))
    {
        // FIN acked
        s->err = 0;
        s->abort();
        return false;
    }

    return true;
}

bool StreamReceiver::
StateClosing::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    u16    urg = ntohs(tcphdr->urg);
    long   offset = tcphdr->getHdrSize();
    long   len = s->getSegmentLength(m, offset, flag);

    if (flag & TCPHdr::RST)
    {
        s->reset(seq);
        return false;
    }

    if (!s->trim(flag, seq, urg, len, offset))
    {
        return true;
    }

    if (flag & TCPHdr::SYN)
    {
        s->sendReset(m);
        s->abort();
        return false;
    }

    if (!(flag & TCPHdr::ACK))
    {
        return false;
    }

    if (s->sendMax < ack)
    {
        // The ACK acks something not yet sent.
        s->ackNow = true;
        return true;
    }
    if (!s->option(tcphdr))
    {
        return false;
    }
    if (s->ack(seq, ack, ntohs(tcphdr->win), ack - s->sendUna, len))
    {
        // FIN acked
        s->err = 0;
        s->setState(stateTimeWait);
    }

    return true;
}

bool StreamReceiver::
StateTimeWait::input(InetMessenger* m, StreamReceiver* s)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    u16 flag = ntohs(tcphdr->flag);
    TCPSeq seq = ntohl(tcphdr->seq);
    TCPSeq ack = ntohl(tcphdr->ack);
    u16    urg = ntohs(tcphdr->urg);
    long   offset = tcphdr->getHdrSize();
    long   len = s->getSegmentLength(m, offset, flag);

    if (flag & TCPHdr::RST)
    {
        s->reset(seq);
        return false;
    }

    if (flag & TCPHdr::SYN)
    {
        // RFC 1122 4.2.2.13
        if (s->recvNext <= seq && !(flag & TCPHdr::ACK))
        {
            // SYN is not an old duplicate.
            s->abort();
            return false;
        }
    }

    if (!s->trim(flag, seq, urg, len, offset))
    {
        return true;
    }

    if (flag & TCPHdr::SYN)
    {
        s->sendReset(m);
        s->abort();
        return false;
    }

    if (flag & TCPHdr::SYN)
    {
        s->sendReset(m);
        s->abort();
        return false;
    }

    if (!(flag & TCPHdr::ACK))
    {
        return false;
    }

    if (s->sendMax < ack)
    {
        // The ACK acks something not yet sent.
        s->ackNow = true;
        return true;
    }
    if (!s->option(tcphdr))
    {
        return false;
    }
    s->ack(seq, ack, ntohs(tcphdr->win), ack - s->sendUna, len);

    // Restart the 2 MSL timeout.
    s->rto = 2 * MSL;
    s->resetRxmitTimer();

    return true;
}
