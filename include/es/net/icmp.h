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

#ifndef NINTENDO_ES_NET_ICMP_H_INCLUDED
#define NINTENDO_ES_NET_ICMP_H_INCLUDED

#include <es/net/inet4.h>

struct ICMPHdr
{
    u8       type;      // ICMP_*
    u8       code;      // code
    u16      sum;
};

struct ICMPEcho
{
    u8       type;      // ICMP_ECHO_REPLY / ICMP_ECHO_REQUEST
    u8       code;      // code
    u16      sum;
    u16      id;
    u16      seq;
};

struct ICMPUnreach
{
    u8       type;      // ICMP_UNREACH
    u8       code;      // code
    u16      sum;
    u16      unused;    // must be zero
    u16      mtu;       // RFC 1191
};

struct ICMPSourceQuench
{
    u8       type;      // ICMP_SOURCE_QUENCH
    u8       code;
    u16      sum;
    u32      unused;    // must be zero
};

struct ICMPRedirect
{
    u8       type;      // ICMP_REDIRECT
    u8       code;
    u16      sum;
    InAddr   gateway;
};

struct ICMPTimeExceeded
{
    u8       type;      // ICMP_TIME_EXCEEDED
    u8       code;      // code (0: ttl reaches zero. 1: fragment timeout)
    u16      sum;
    u32      unused;    // must be zero
};

struct ICMPParamProb
{
    u8       type;      // ICMP_PARAM_PROB
    u8       code;
    u16      sum;
    u8       ptr;
    u8       unused0;
    u8       unused1;
    u8       unused2;
};

#define ICMP_MIN_HLEN                   4

// ICMP message type
#define ICMP_ECHO_REPLY                 0   // [MUST]
#define ICMP_UNREACH                    3
#define ICMP_SOURCE_QUENCH              4   // [Host MAY send, MUST report to xport layer]
#define ICMP_REDIRECT                   5   // [Host SHOULD NOT send, MUST update its routing info]
#define ICMP_ECHO_REQUEST               8
#define ICMP_TIME_EXCEEDED              11  // [MUST be passed to xport layer]
#define ICMP_PARAM_PROB                 12  // [MUST be passed to xport layer]
#define ICMP_TIMESTAMP_REQUEST          13  // A host MAY implement Timestamp [RFC1122]
#define ICMP_TIMESTAMP_REPLY            14  // A host MAY implement Timestamp Reply [RFC1122]
#define ICMP_ADDRESS_MASK_REQUEST       17  // [RFC 950]
#define ICMP_ADDRESS_MASK_REPLY         18  // [RFC 950]

// ICMP_REDIRECT codes
#define ICMP_REDIRECT_NET               0   // Redirect for network error
#define ICMP_REDIRECT_HOST              1   // Redirect for host error
#define ICMP_REDIRECT_TOSNET            2   // Redirect for TOS and network error
#define ICMP_REDIRECT_TOSHOST           3   // Redirect for TOS and host error

// ICMP_UNREACH codes
#define ICMP_UNREACH_NET                0   // hint
#define ICMP_UNREACH_HOST               1   // hint
#define ICMP_UNREACH_PROTOCOL           2   // protocol [Host SHOULD generate]
#define ICMP_UNREACH_PORT               3   // port     [Host SHOULD generate]
#define ICMP_UNREACH_NEED_FRAGMENT      4   // fragmentation needed and DF set
#define ICMP_UNREACH_SRC_FAIL           5   // hint Bad source route
#define ICMP_UNREACH_NET_UNKNOWN        6
#define ICMP_UNREACH_HOST_UNKNOWN       7
#define ICMP_UNREACH_ISOLATED           8   // obsolete
#define ICMP_UNREACH_NET_PROHIBITED     9   // intended for use by U.S military agencies
#define ICMP_UNREACH_HOST_PROHIBITED    10  // intended for use by U.S military agencies
#define ICMP_UNREACH_NET_TOS            11
#define ICMP_UNREACH_HOST_TOS           12
#define ICMP_UNREACH_PROHIBITED         13

#endif  // NINTENDO_ES_NET_ICMP_H_INCLUDED
