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

#ifndef STREAM_H_INCLUDED
#define STREAM_H_INCLUDED

#include <algorithm>
#include <es/dateTime.h>
#include <es/endian.h>
#include <es/ring.h>
#include <es/timer.h>
#include <es/synchronized.h>
#include <es/base/IMonitor.h>
#include <es/net/inet4.h>
#include <es/net/inet6.h>
#include <es/net/tcp.h>
#include "inet.h"
#include "socket.h"

#define TCP_SACK
#define TCP_LIMITED_TRANSMIT

class StreamReceiver :
    public SocketReceiver
{
    static const int IIS_CLOCK = 1000000/4;     // Initial sequence number frequency [Hz]
    static const int DEF_SSTHRESH = 65535;      // Default slow start threshold
    static const int R1 = 3;                    // At least 3 retransmissions [RFC 1122]
    static const int PMTUD_BACKOFF = 4;         // Path MTU discovery blackhole detection
    static const int MAX_BACKOFF = 16;          // Maximum retransmission count allowed.
                                                // Must be more than R1, add less than 31.
    static const int RXMIT_THRESH;              // Fast restransmission threshold
    static const int LIMITED_THRESH = 2;        // Limited Transmit threshold

    static const TimeSpan R2;
    static const TimeSpan R2_SYN;
    static const TimeSpan MSL;                  // Maximum segment lifetime
    static const TimeSpan RTT_MIN;              // Minimum restransmission timeout. SHOULD be 1 second [RFC 2988]
    static const TimeSpan RTT_MAX;              // Maximum restransmission timeout.
    static const TimeSpan RTT_DEFAULT;          // Initial restransmission timeout for SYN.
    static const TimeSpan PERSIST_MAX;          // Maximum idle time in persist state
    static const TimeSpan DACK_TIMEOUT;         // Delayed ACK timeout

    class RxmitTimer : public TimerTask
    {
        StreamReceiver* receiver;
    public:
        RxmitTimer(StreamReceiver* receiver) :
            receiver(receiver)
        {}
        void run()
        {
            receiver->expired();
        }
    };

    class AckTimer : public TimerTask
    {
        StreamReceiver* receiver;
    public:
        AckTimer(StreamReceiver* receiver) :
            receiver(receiver)
        {}
        void run()
        {
            receiver->delayedAck();
        }
    };

    struct SackHole
    {
        TCPSeq  start;      // start seq no. of hole
        TCPSeq  end;        // end seq no.
        int     dupAcks;    // number of dup acks for this hole
        TCPSeq  rxmit;      // next seq. no in hole to be retransmitted
    };

    class State
    {
    public:
        virtual void start(StreamReceiver* s)
        {
        }
        virtual void abort(StreamReceiver* s)
        {
            s->abort();
        }
        virtual bool input(InetMessenger* m, StreamReceiver* s)
        {
            return true;
        }
        virtual bool output(InetMessenger* m, StreamReceiver* s)
        {
            return true;
        }
        virtual bool error(InetMessenger* m, StreamReceiver* s)
        {
            return true;
        }
        virtual bool accept(SocketMessenger* m, StreamReceiver* s)
        {
            return false;
        }
        virtual bool connect(SocketMessenger* m, StreamReceiver* s)
        {
            return false;
        }
        virtual bool close(SocketMessenger* m, StreamReceiver* s)
        {
            return false;
        }
        virtual bool hasBeenEstablished()
        {
            return true;
        }
        virtual const char* getName() const = 0;
    };

    class StateClosed : public State
    {
    public:
        void abort(StreamReceiver* s)
        {
        }
        bool input(InetMessenger* m, StreamReceiver* s);
        bool connect(SocketMessenger* m, StreamReceiver* s);
        bool hasBeenEstablished()
        {
            return false;
        }
        const char* getName() const
        {
            return "Closed";
        }
    };

    class StateListen : public State
    {
    public:
        bool input(InetMessenger* m, StreamReceiver* s);
        bool accept(SocketMessenger* m, StreamReceiver* s);
        bool close(SocketMessenger* m, StreamReceiver* s)
        {
            s->abort();
            return false;
        }
        bool hasBeenEstablished()
        {
            return false;
        }
        const char* getName() const
        {
            return "Listen";
        }
    };

    class StateSynSent : public State
    {
    public:
        bool input(InetMessenger* m, StreamReceiver* s);
        bool output(InetMessenger* m, StreamReceiver* s);
        bool close(SocketMessenger* m, StreamReceiver* s)
        {
            s->abort();
            return false;
        }
        bool hasBeenEstablished()
        {
            return false;
        }
        const char* getName() const
        {
            return "SynSent";
        }
    };

    class StateSynReceived : public State
    {
    public:
        void abort(StreamReceiver* s)
        {
            s->sendReset();
            s->abort();
        }
        bool input(InetMessenger* m, StreamReceiver* s);
        bool output(InetMessenger* m, StreamReceiver* s);
        bool close(SocketMessenger* m, StreamReceiver* s)
        {
            // Note TCP still needs to transmit SYN before FIN
            // to switch to stateFinWait1.
            return true;
        }
        bool hasBeenEstablished()
        {
            return false;
        }
        const char* getName() const
        {
            return "SynReceived";
        }
    };

    class StateEstablished : public State
    {
    public:
        void start(StreamReceiver* s)
        {
            s->r2 = R2;
        }
        void abort(StreamReceiver* s)
        {
            s->sendReset();
            s->abort();
        }
        bool input(InetMessenger* m, StreamReceiver* s);
        bool output(InetMessenger* m, StreamReceiver* s);
        bool close(SocketMessenger* m, StreamReceiver* s)
        {
            s->setState(stateFinWait1);
            return true;
        }
        const char* getName() const
        {
            return "Established";
        }
    };

    class StateFinWait1 : public State
    {
    public:
        void abort(StreamReceiver* s)
        {
            s->sendReset();
            s->abort();
        }
        bool input(InetMessenger* m, StreamReceiver* s);
        bool output(InetMessenger* m, StreamReceiver* s);
        const char* getName() const
        {
            return "FinWait1";
        }
    };

    class StateFinWait2 : public State
    {
    public:
        void abort(StreamReceiver* s)
        {
            s->sendReset();
            s->abort();
        }
        bool input(InetMessenger* m, StreamReceiver* s);
        bool output(InetMessenger* m, StreamReceiver* s);
        const char* getName() const
        {
            return "FinWait2";
        }
    };

    class StateCloseWait : public State
    {
    public:
        void start(StreamReceiver* s)
        {
            s->shutrd = true;
            s->notify();
        }
        void abort(StreamReceiver* s)
        {
            s->sendReset();
            s->abort();
        }
        bool input(InetMessenger* m, StreamReceiver* s);
        bool output(InetMessenger* m, StreamReceiver* s);
        bool close(SocketMessenger* m, StreamReceiver* s)
        {
            s->setState(stateLastAck);
            return true;
        }
        const char* getName() const
        {
            return "CloseWait";
        }
    };

    class StateLastAck : public State
    {
    public:
        bool input(InetMessenger* m, StreamReceiver* s);
        bool output(InetMessenger* m, StreamReceiver* s);
        const char* getName() const
        {
            return "LastAck";
        }
    };

    class StateClosing : public State
    {
    public:
        void start(StreamReceiver* s)
        {
            s->shutrd = true;
            s->notify();
        }
        bool input(InetMessenger* m, StreamReceiver* s);
        bool output(InetMessenger* m, StreamReceiver* s);
        const char* getName() const
        {
            return "Closing";
        }
    };

    class StateTimeWait : public State
    {
    public:
        void start(StreamReceiver* s)
        {
            s->stopRxmitTimer();
            s->stopAckTimer();
            s->shutrd = true;

            s->rto = 2 * MSL;
            s->startRxmitTimer();

            s->notify();
        }
        bool input(InetMessenger* m, StreamReceiver* s);
        bool output(InetMessenger* m, StreamReceiver* s);
        const char* getName() const
        {
            return "TimeWait";
        }
    };

    //
    // Implementation specific members:
    //

    State*      state;
    es::Monitor*   monitor;
    u8*         recvBuf;
    Ring        recvRing;
    u8*         sendBuf;
    Ring        sendRing;
    Conduit*    conduit;
    int         err;
    Socket*     socket;

    //
    // Standard members from RFC 793, RFC2018, etc.:
    //

    s32         mss;        // maximum segment size (might be reduced by PMTU discovery)
    bool        persist;
    bool        nagle;
    bool        shutrd;
    bool        shutwr;
    bool        haveUrg;
    bool        hadUrg;
    bool        ackNow;
    bool        fastRxmit;
    bool        sack;

    // Send Sequence Variables
    TCPSeq      sendUna;    // send unacknowledged
    TCPSeq      sendNext;   // send next
    TCPSeq      sendUp;     // send urgent pointer
    TCPSeq      sendWL1;    // segment sequence number used for last window update
    TCPSeq      sendWL2;    // segment acknowledgment number used for a window update
    TCPSeq      iss;        // initial send sequence number
    s32         sendMaxWin; // the maximum send window size so far [Max(SND.WND) RFC1122]
    TCPSeq      sendMax;    // send max
    TCPSeq      sendFack;   // for FACK congestion control

    s32         sendWin;    // send window
    s32         sendAwin;   // sendNext - sendFack + rxmitData

    // Receive Sequence Variables
    TCPSeq      recvNext;   // receive next
    s32         recvWin;    // receive window
    TCPSeq      recvUp;     // receive urgent pointer
    TCPSeq      irs;        // initial receive sequence number
    TCPSeq      recvAcked;
    s32         dupAcks;    // # of duplicated ACKs received.
    Ring::Vec   asb[TCPHdr::ASB_MAX];   // above sequence blocks received

    // Slow start, Congestion avoidance
    s32         cWin;       // Congestion window size. The congestion
                            // window is a count of how many bytes will
                            // fit in the pipe.
    s32         ssThresh;   // Slow start threshold.
    s32         cAcked;     // Count of acked bytes during congestion avoidance

    // Round trip timing
    DateTime    rttTiming;  // Non-zero while measuring RTT value
    TCPSeq      rttSeq;     // Sequence number when rttTiming is set
    TimeSpan    srtt;       // Smoothed round trip time estimate
    s64         rttDe;      // Smoothed mean deviation estimator
    DateTime    lastSend;   // Time when the last packet was sent used when re-starting the idle connection.

    // Retransmission
    s32         rxmitCount; // Count of consecutive resransmissions
    TimeSpan    rto;        // Retransmission timeout value
    DateTime    r0;         //
    TimeSpan    r2;         // When transmissions reaches R2, close the connection. [RFC 1122 4.2.3.5]
    RxmitTimer  rxmitTimer; // restransmision timer
    s32         rxmitData;  // amount of outstanding rxmit data

    // Delayed ACK
    AckTimer    ackTimer;

    // SACK/FACK
    SackHole    scoreboard[TCPHdr::ASB_MAX];
                                // list of non-SACKed holes
    int         sendHoles;      // # of holes in scoreboard
    TCPSeq      sendRecover;    // sendNext at the time the first loss was detected
    TCPSeq      lastSack;
    TCPSeq      onxt;
    SackHole*   hole;

    // Listen/Accept
    StreamReceiver*                             listening;  // listening socket
    Link<StreamReceiver>                        link;
    List<StreamReceiver, &StreamReceiver::link> accepted;

    TCPSeq isn(InetMessenger* m);
    int getDefaultMSS();
    int getDefaultMSS(int mtu);

    s32 getInitialCongestionWindowSize()
    {
        // RFC 2581 allows a TCP to use an initial cwnd of up to 2 segments.
        return 2 * mss;
    }

    int countOptionSize(u16 flag);
    int fillOptions(u8* opt, u16 flag);

    void setPersist(bool persist)
    {
        this->persist = persist;
    }
    bool isPersist()
    {
        return persist;
    }

    bool isShutdownOutput()
    {
        return shutwr;
    }
    bool isShutdownInput()
    {
        return shutrd;
    }

    //
    // Input
    //

    static long getSegmentLength(InetMessenger* m, long offset, u16 flag)
    {
        long len = m->getLength() - offset;
        // SYN is considered to occur before the first actual data
        if (flag & TCPHdr::SYN)
        {
            ++len;
        }
        // FIN is considered to occur after the last actual data
        if (flag & TCPHdr::FIN)
        {
            ++len;
        }
        return len;
    }

    bool trim(u16& flag, TCPSeq& seq, u16& urg, long& len, long& offset);

    void reset(TCPSeq seq, int err = 0)
    {
        // Note segment data in RSTs is an ASCII text that encoded
        // and explained the cause of the RST [RFC 793]. It is not a
        // part of the data stream. Assume the segment length is zero.
        if (0 == recvWin && seq == recvNext ||
            0 < recvWin && recvNext <= seq && seq < TCPSeq(recvNext + recvWin))
        {
            if (err)
            {
                this->err = err;
            }
            abort();
        }
        // else RSTs didn't pass the sequence number test. Just drop it
        // since ACK must not be send back for RSTs.
    }

    void openWindow(TCPSeq ack);
    bool ack(TCPSeq seq, TCPSeq ack, s32 win, s32 sent, long len);
    void urg(InetMessenger* m, TCPSeq seq, u16 urg, long len);
    bool text(InetMessenger* m, u16& flag, TCPSeq seq, long len, long offset);
    bool option(TCPHdr* tcphdr);

    //
    // Output
    //
    s32 getSendableWithSyn(u16& flag);
    s32 getSendable();
    s32 getSendableWithFin(u16& flag);
    bool canSend(s32 len, s32 mss, u16 flag);
    bool send(InetMessenger* m, s32 sendable, u16 flag);
    void sendReset(InetMessenger* m);
    void sendReset();

    //
    // Timer
    //

    void startRxmitTimer();
    void stopRxmitTimer();
    void resetRxmitTimer();

    void initRto();
    void updateRto(TimeSpan rtt);
    void cutThresh();

    void startAckTimer();
    void stopAckTimer();
    void delayedAck();

    //
    // Sack
    //
    SackHole* getSackHole();
    void deleteSackHoles(TCPSeq ack);
    void updateScoreboard(TCPSeq ack, TCPOptSack* optSack);

    bool isAcceptable()
    {
        return state != &stateListen || !accepted.isEmpty() || err;
    }
    bool isConnectable()
    {
        return state != &stateSynSent || err;
    }
    bool isReadable()
    {
        return 0 < recvRing.getUsed() || isShutdownInput() || err;
    }
    bool isWritable()
    {
        return 0 < socket->getSendBufferSize() - sendRing.getUsed() ||
               isShutdownOutput() || err;
    }
    bool isClosable()
    {
        return state == &stateClosed || state == &stateTimeWait;
    }

public:
    StreamReceiver(Conduit* conduit = 0) :
        state(&stateClosed),
        monitor(0),
        recvBuf(0),
        sendBuf(0),
        conduit(conduit),
        err(0),
        socket(0),

        mss(576 - sizeof(IPHdr) - sizeof(TCPHdr)),  // XXX v4 specific
        persist(false),
        nagle(false),
        shutrd(false),
        shutwr(false),
        haveUrg(false),
        hadUrg(false),
        ackNow(false),
        fastRxmit(false),
        sack(false),

        sendWin(mss),
        recvWin(recvRing.getSize()),

        dupAcks(0),

        cWin(2 * mss),  // RFC 2581 allows a TCP to use an initial cwnd of up to 2 segments.
        ssThresh(DEF_SSTHRESH),
        cAcked(0),

        rxmitTimer(this),

        ackTimer(this),

        listening(0)
    {
        monitor = es::Monitor::createInstance();

        initRto();
    }

    ~StreamReceiver()
    {
        if (recvBuf)
        {
            delete[] recvBuf;
        }
        if (sendBuf)
        {
            delete[] sendBuf;
        }
        if (monitor)
        {
            monitor->release();
        }
    }

    State* getState()
    {
        return state;
    }

    void setState(State& state)
    {
        if (this->state != &state)
        {
            esReport("State: %s\n", state.getName());

            this->state = &state;
            state.start(this);
        }
    }

    bool initialize(Socket* socket)
    {
        this->socket = socket;
        recvBuf = new u8[socket->getReceiveBufferSize()];
        recvRing.initialize(recvBuf, socket->getReceiveBufferSize());
        sendBuf = new u8[socket->getSendBufferSize()];
        sendRing.initialize(sendBuf, socket->getSendBufferSize());
        return true;
    }

    void notify()
    {
        monitor->notifyAll();
        if (socket->selector)
        {
            socket->selector->notifyAll();
        }
    }

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
    bool error(InetMessenger* m, Conduit* c);

    bool read(SocketMessenger* m, Conduit* c);
    bool write(SocketMessenger* m, Conduit* c);

    bool accept(SocketMessenger* m, Conduit* c);
    bool listen(SocketMessenger* m, Conduit* c);
    bool connect(SocketMessenger* m, Conduit* c);
    bool close(SocketMessenger* m, Conduit* c);
    bool shutdownOutput(SocketMessenger* m, Conduit* c);
    bool shutdownInput(SocketMessenger* m, Conduit* c);

    bool isAcceptable(SocketMessenger* m, Conduit* c)
    {
        m->setErrorCode(isAcceptable());
        return false;
    }

    bool isConnectable(SocketMessenger* m, Conduit* c)
    {
        m->setErrorCode(isConnectable());
        return false;
    }

    bool isReadable(SocketMessenger* m, Conduit* c)
    {
        m->setErrorCode(isReadable());
        return false;
    }

    bool isWritable(SocketMessenger* m, Conduit* c)
    {
        m->setErrorCode(isWritable());
        return false;
    }

    bool atMark(SocketMessenger* m, Conduit* c)
    {
        if (!hadUrg && haveUrg && (recvRing.getUsed() + recvUp - recvNext == 0))
        {
            m->setFlag(true);
            return false;
        }
        m->setFlag(false);
        return false;
    }

    void expired();
    void abort();

    StreamReceiver* clone(Conduit* conduit, void* key)
    {
        return new StreamReceiver(conduit);
    }

    unsigned int release()
    {
        delete this;
        return 0;
    }

    static class StateClosed        stateClosed;
    static class StateListen        stateListen;
    static class StateSynSent       stateSynSent;
    static class StateSynReceived   stateSynReceived;
    static class StateEstablished   stateEstablished;
    static class StateFinWait1      stateFinWait1;
    static class StateFinWait2      stateFinWait2;
    static class StateCloseWait     stateCloseWait;
    static class StateLastAck       stateLastAck;
    static class StateClosing       stateClosing;
    static class StateTimeWait      stateTimeWait;
};

#endif  // STREAM_H_INCLUDED
