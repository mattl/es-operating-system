/*
 * Copyright 2008 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_NET_TCP_H_INCLUDED
#define NINTENDO_ES_NET_TCP_H_INCLUDED

#include <es.h>
#include <es/endian.h>
#include <es/ring.h>

//
// TCP RFC 793
//

class TCPSeq
{
    s32 seq;    // sequence number
public:
    TCPSeq(s32 seq = 0) : seq(seq)
    {
    }

    TCPSeq& operator=(s32 seq)
    {
        this->seq = seq;
        return *this;
    }

    TCPSeq& operator+=(s32 octets)
    {
        seq += octets;
        return *this;
    }

    TCPSeq& operator++()
    {
        ++seq;
        return *this;
    }

    operator s32() const
    {
        return seq;
    }
};

inline bool operator<(TCPSeq a, TCPSeq b)
{
    return 0 < b - a;
}

inline bool operator<=(TCPSeq a, TCPSeq b)
{
    return 0 <= b - a;
}

inline bool operator>(TCPSeq a, TCPSeq b)
{
    return 0 < a - b;
}

inline bool operator>=(TCPSeq a, TCPSeq b)
{
    return 0 <= a - b;
}

struct TCPHdr
{
    static const int MIN_HLEN = 20;
    static const int MAX_HLEN = 60;

    static const u16 FIN = 0x0001;      // No more data from sender
    static const u16 SYN = 0x0002;      // Synchronize sequence numbers
    static const u16 RST = 0x0004;      // Reset the connection
    static const u16 PSH = 0x0008;      // Push function
    static const u16 ACK = 0x0010;      // Acknowledgment field significant
    static const u16 URG = 0x0020;      // Urgent pointer field significant

    static const u16 ECE = 0x0040;      // [RFC 3168]
    static const u16 CWR = 0x0080;      // [RFC 3168]

    // Option kind
    static const u8 OPT_EOL = 0;        // End of option list
    static const u8 OPT_NOP = 1;        // No-Operation.
    static const u8 OPT_MSS = 2;        // Maximum Segment Size (length = 4, can only apper in SYNs)

    // RFC 1323 TCP Extensions for High Performance
    static const u8 OPT_WS = 3;         // Window scale (length = 3)
    static const u8 OPT_TS = 8;         // Timestamps (length = 10)

    // RFC 2018 TCP Selective Acknowledgement Options
    static const u8 OPT_SACKP = 4;      // Sack-Permitted (length = 2)
    static const u8 OPT_SACK = 5;       // Sack (length = variable)
    static const int ASB_MAX = 4;       // 8 * ASB_MAX + 2 < 40

    u16 src;    // source port
    u16 dst;    // destination port
    s32 seq;    // sequence number
    s32 ack;    // acknowledgment number
    u16 flag;   // 4:data offset, 4:reserved, 1:CWR, 1:ECE,
                // 1:URG, 1:ACK, 1:PSH, 1:RST, 1:SYN, 1:FIN
    u16 win;    // window
    u16 sum;    // checksum
    u16 urg;    // urgent pointer

    // option (if any)

    // data (if any)

    int getHdrSize() const
    {
        u16 size = ntohs(flag);
        return ((size & 0xf000) >> 10);
    }

    void setHdrSize(int size)
    {
        ASSERT(MIN_HLEN <= size && size <= MAX_HLEN && (size & 3) == 0);
        flag &= htons(0x0fff);
        flag |= htons(size << 10);
    }
};

struct TCPOpt
{
    u8  kind;
};

struct TCPOptEol
{
    u8  kind;

    TCPOptEol() : kind(TCPHdr::OPT_EOL) {}
};

struct TCPOptNop
{
    u8  kind;

    TCPOptNop() : kind(TCPHdr::OPT_NOP) {}
};

struct TCPOptMss
{
    u8  kind;
    u8  len;
    u16 mss;

    TCPOptMss(u16 mss) :
        kind(TCPHdr::OPT_MSS),
        len(4),
        mss(htons(mss))
    {
    }

    int getMSS()
    {
        return ntohs(mss);
    }
};

struct TCPOptSackPermitted
{
    u8  kind;
    u8  len;

    TCPOptSackPermitted() :
        kind(TCPHdr::OPT_SACKP),
        len(2)
    {
    }
};

struct TCPOptSack
{
    u8  kind;
    u8  len;
    struct
    {
        s32 left;
        s32 right;
    }   edge[1];

    TCPOptSack(u8* head, long size, Ring::Vec* asb, s32 offset) :
        kind(TCPHdr::OPT_SACK)
    {
        Ring::Vec* block;
        int i;
        for (block = &asb[TCPHdr::ASB_MAX - 1], i = 0; asb <= block; --block, ++i)
        {
            if (block->data)
            {
                s32 left;
                s32 right;

                if (head <= block->data)
                {
                    left = (u8*) block->data - head;
                }
                else
                {
                    left = (u8*) block->data + size - head;
                }
                left += offset;
                right = left + block->count;

                edge[i].left = htonl(left);
                edge[i].right = htonl(right);
            }
        }
        len = 8 * i + 2;
    }
};

#endif  // NINTENDO_ES_NET_TCP_H_INCLUDED
