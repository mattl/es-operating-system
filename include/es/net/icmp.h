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

#ifndef NINTENDO_ES_NET_ICMP_H_INCLUDED
#define NINTENDO_ES_NET_ICMP_H_INCLUDED

#include <es/net/inet4.h>

struct ICMPHdr
{
    u8       type;
    u8       code;
    u16      sum;

    // Message type
    static const int EchoReply = 0;             // [MUST]
    static const int Unreach = 3;
    static const int SourceQuench = 4;          // [Host MAY send, MUST report to xport layer]
    static const int Redirect = 5;              // [Host SHOULD NOT send, MUST update its routing info]
    static const int EchoRequest = 8;
    static const int TimeExceeded = 11;         // [MUST be passed to xport layer]
    static const int ParamProb = 12;            // [MUST be passed to xport layer]
    static const int TimestampRequest = 13;     // A host MAY implement Timestamp [RFC1122]
    static const int TimestampReply = 14;       // A host MAY implement Timestamp Reply [RFC1122]
    static const int AddressMaskRequest = 17;   // [RFC 950]
    static const int AddressMaskReply = 18;     // [RFC 950]
};

struct ICMPEcho
{
    u8       type;      // EchoReply or EchoRequest
    u8       code;
    u16      sum;
    u16      id;
    u16      seq;
};

struct ICMPUnreach
{
    u8       type;      // Unreach
    u8       code;
    u16      sum;
    u16      unused;    // must be zero
    u16      mtu;       // RFC 1191

    int getMTU() const
    {
        return ntohs(mtu);
    }

    void setMTU(int mtu)
    {
        ASSERT(0 <= mtu && mtu < 65536);
        this->mtu = htons(mtu);
    }

    // Unreach. codes
    static const int Net = 0;               // hint
    static const int Host = 1;              // hint
    static const int Protocol = 2;          // protocol [Host SHOULD generate]
    static const int Port = 3;              // port [Host SHOULD generate]
    static const int NeedFragment = 4;      // fragmentation needed and DF set
    static const int SrcFail = 5;           // hint Bad source route
    static const int NetUnknown = 6;
    static const int HostUnknown = 7;
    static const int Isolated = 8;          // obsolete
    static const int NetProhibited = 9;     // intended for use by U.S military agencies
    static const int HostProhibited = 10;   // intended for use by U.S military agencies
    static const int NetTOS = 11;
    static const int HostTOS = 12;
    static const int Prohibited = 13;
};

struct ICMPSourceQuench
{
    u8       type;      // SourceQuench
    u8       code;
    u16      sum;
    u32      unused;    // must be zero
};

struct ICMPRedirect
{
    u8       type;      // Redirect
    u8       code;
    u16      sum;
    InAddr   gateway;

    // Redirect codes
    static const int Net = 0;       // network error
    static const int Host = 1;      // host error
    static const int NetTOS = 2;    // TOS and network error
    static const int HostTOS = 3;   // TOS and host error
};

struct ICMPTimeExceeded
{
    u8       type;      // TimeExceeded
    u8       code;      // 0: ttl reaches zero. 1: fragment timeout
    u16      sum;
    u32      unused;    // must be zero

    // Codes:
    static const u8 TTLReachesZero = 0;
    static const u8 FragmentTimeout = 1;
};

struct ICMPParamProb
{
    u8       type;      // ParamProb
    u8       code;
    u16      sum;
    u8       ptr;
    u8       unused0;
    u8       unused1;
    u8       unused2;
};

#endif  // NINTENDO_ES_NET_ICMP_H_INCLUDED
