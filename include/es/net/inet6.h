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

#ifndef NINTENDO_ES_NET_INET6_H_INCLUDED
#define NINTENDO_ES_NET_INET6_H_INCLUDED

#include <string.h>
#include <es/types.h>

//
// RFC 3493
//

#define INET6_ADDRSTRLEN    46

// Next header numbers (IPv4 protocol numbers)
#define IPPROTO_HOPOPTS     0    // IPv6 Hop-by-Hop options
#define IPPROTO_IPV6        41   // IPv6 header
#define IPPROTO_ROUTING     43   // IPv6 Routing header
#define IPPROTO_FRAGMENT    44   // IPv6 fragment header
#define IPPROTO_ESP         50   // encapsulating security payload
#define IPPROTO_AH          51   // authentication header
#define IPPROTO_ICMPV6      58   // ICMPv6
#define IPPROTO_NONE        59   // IPv6 no next header
#define IPPROTO_DSTOPTS     60   // IPv6 Destination options

#define IPV6_V6ONLY         0
#define IPV6_UNICAST_HOPS   4
#define IPV6_MULTICAST_IF   9
#define IPV6_MULTICAST_HOPS 10
#define IPV6_MULTICAST_LOOP 11
#define IPV6_JOIN_GROUP     12
#define IPV6_LEAVE_GROUP    13

#define AF_INET6            23
#define PF_INET6            AF_INET6

#define IN6ADDR_ANY_INIT                     { 0 }
#define IN6ADDR_LOOPBACK_INIT                { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
#define IN6ADDR_LINKLOCAL_ALL_NODES_INIT     { 0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
#define IN6ADDR_LINKLOCAL_ALL_ROUTERS_INIT   { 0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,2 }

struct In6Addr
{
    u8          addr[16];   // IPv6 address
};

inline int operator==(const In6Addr& a, const In6Addr& b)
{
    return (memcmp(&a.addr, &b.addr, 16) == 0) ? true : false;
}

inline int operator!=(const In6Addr& a, const In6Addr& b)
{
    return (memcmp(&a.addr, &b.addr, 16) != 0) ? true : false;
}

struct SockAddrIn6
{
    u8          len;        // length of this struct
    u8          family;     // AF_INET6
    u16         port;       // transport layer port #
    u32         flowInfo;   // IPv6 flow information
    In6Addr     addr;       // IPv6 address
    u32         scopeID;    // set of interfaces for a scope
};

struct IPv6Mreq
{
    In6Addr     multiAddr;  // IPv6 multicast addr
    u32         interface;  // interface index
};


//
// RFC 3542
//

struct IP6Hdr
{
    u32         flow;       // 4 bits version, 8 bits TC, 20 bits flow-ID
    u16         plen;       // payload length
    u8          next;       // next header
    u8          hops;       // hop limit
    In6Addr     src;        // source address
    In6Addr     dst;        // destination address
};

//
// IPv6 Extension Headers
//

// Hop-by-Hop options header
struct IP6Hbh
{
    u8          next;       // next header
    u8          len;        // length in units of 8 octets
    // followed by options
};

// Destination options header
struct IP6Dest
{
    u8          next;       // next header
    u8          len;        // length in units of 8 octets
    // followed by options
};

// Routing header
struct IP6Rthdr
{
    u8          next;       // next header
    u8          len;        // length in units of 8 octets
    u8          type;       // routing type
    u8          segleft;    // segments left
    // followed by routing type specific data
};

// Type 0 Routing header
struct IP6Rthdr0
{
    u8          next;       // next header
    u8          len;        // length in units of 8 octets
    u8          type;       // always zero
    u8          segleft;    // segments left
    u8          reserved;   // reserved field
    // followed by up to 127 struct in6_addr
};

// Fragment header
struct IP6Frag
{
    u8          next;       // next header
    u8          reserved;   // reserved field
    u16         offlg;      // offset, reserved, and flag
    u32         ident;      // identification
};

#define IP6F_OFF_MASK       0xfff8  // mask out offset from ip6f_offlg
#define IP6F_RESERVED_MASK  0x0006  // reserved bits in ip6f_offlg
#define IP6F_MORE_FRAG      0x0001  // more-fragments flag

//
// IPv6 Options
//

// IPv6 options
struct IP6Opt
{
    u8          type;
    u8          len;
};

#define IP6OPT_TYPE(o)              ((o) & 0xc0)
#define IP6OPT_TYPE_SKIP            0x00
#define IP6OPT_TYPE_DISCARD         0x40
#define IP6OPT_TYPE_FORCEICMP       0x80
#define IP6OPT_TYPE_ICMP            0xc0
#define IP6OPT_MUTABLE              0x20

#define IP6OPT_PAD1                 0x00  // 00 0 00000
#define IP6OPT_PADN                 0x01  // 00 0 00001
#define IP6OPT_JUMBO                0xc2  // 11 0 00010
#define IP6OPT_NSAP_ADDR            0xc3  // 11 0 00011
#define IP6OPT_TUNNEL_LIMIT         0x04  // 00 0 00100
#define IP6OPT_ROUTER_ALERT         0x05  // 00 0 00101

// Jumbo Payload Option
struct IP6OptJumbo
{
    u8          type;
    u8          len;
    u8          jumboLen[4];
};

#define IP6OPT_JUMBO_LEN            6

// NSAP Address Option
struct IP6OptNsap
{
    u8          type;
    u8          len;
    u8          srcNsapLen;
    u8          dstNsapLen;
    // followed by source NSAP
    // followed by destination NSAP
};

// Tunnel Limit Option
struct IP6OptTunnel
{
    u8          type;
    u8          len;
    u8          encapLimit;
};

// Router Alert Option
struct IP6OptRouter
{
    u8          type;
    u8          len;
    u8          value[2];
};

// Router alert values (in network byte order)
#define IP6_ALERT_MLD               0x0000
#define IP6_ALERT_RSVP              0x0001
#define IP6_ALERT_AN                0x0002

// The ICMP6Hdr Structure

struct ICMP6Hdr
{
    u8          type;       // type field
    u8          code;       // code field
    u16         cksum;      // checksum field
    u32         data;
};

// ICMPv6 Type and Code Values

#define ICMP6_DST_UNREACH           1
#define ICMP6_PACKET_TOO_BIG        2
#define ICMP6_TIME_EXCEEDED         3
#define ICMP6_PARAM_PROB            4

#define ICMP6_INFOMSG_MASK          0x80    // all informational messages

#define ICMP6_ECHO_REQUEST          128
#define ICMP6_ECHO_REPLY            129

#define ICMP6_DST_UNREACH_NOROUTE       0 // no route to destination
#define ICMP6_DST_UNREACH_ADMIN         1 // communication with destination admin. prohibited
#define ICMP6_DST_UNREACH_BEYONDSCOPE   2 // beyond scope of source address
#define ICMP6_DST_UNREACH_ADDR          3 // address unreachable
#define ICMP6_DST_UNREACH_NOPORT        4 // bad port

#define ICMP6_TIME_EXCEED_TRANSIT       0 // Hop Limit == 0 in transit
#define ICMP6_TIME_EXCEED_REASSEMBLY    1 // Reassembly time out

#define ICMP6_PARAMPROB_HEADER          0 // erroneous header field
#define ICMP6_PARAMPROB_NEXTHEADER      1 // unrecognized Next Header
#define ICMP6_PARAMPROB_OPTION          2 // unrecognized IPv6 option

//  ICMPv6 Neighbor Discovery Definitions

#define ND_ROUTER_SOLICIT           133
#define ND_ROUTER_ADVERT            134
#define ND_NEIGHBOR_SOLICIT         135
#define ND_NEIGHBOR_ADVERT          136
#define ND_REDIRECT                 137

// router solicitation
struct NDRouterSolicit
{
    u8          type;       // type field
    u8          code;       // code field
    u16         cksum;      // checksum field
    u32         reserved;
    // could be followed by options
};

// router advertisement
struct NDRouterAdvert
{
    u8          type;       // type field
    u8          code;       // code field
    u16         cksum;      // checksum field
    u8          curHopLimit;
    u8          flagsReserved;
    u16         routerLifetime;
    u32         reachable;  // reachable time
    u32         retransmit; // retransmit timer
    // could be followed by options
};

#define ND_RA_FLAG_MANAGED          0x80
#define ND_RA_FLAG_OTHER            0x40

// neighbor solicitation
struct NDNeighborSolicit
{
    u8          type;       // type field
    u8          code;       // code field
    u16         cksum;      // checksum field
    u32         reserved;
    In6Addr     target;     // target address
    // could be followed by options
};

// neighbor advertisement
struct NDNeighborAdvert
{
    u8          type;       // type field
    u8          code;       // code field
    u16         cksum;      // checksum field
    u32         flagsReserved;
    In6Addr     target;     // target address
    // could be followed by options
};

#define ND_NA_FLAG_ROUTER           0x80000000
#define ND_NA_FLAG_SOLICITED        0x40000000
#define ND_NA_FLAG_OVERRIDE         0x20000000

// redirect
struct NDRedirect
{
    u8          type;       // type fiel
    u8          code;       // code field
    u16         cksum;      // checksum field
    u32         reserved;
    In6Addr     target;     // target address
    In6Addr     dst;        // destination address
    // could be followed by options
};

// Neighbor discovery option header
struct NDOptHdr
{
    u8          type;
    u8          len;        // in units of 8 octets
    // followed by option specific data
};

#define  ND_OPT_SOURCE_LINKADDR     1
#define  ND_OPT_TARGET_LINKADDR     2
#define  ND_OPT_PREFIX_INFORMATION  3
#define  ND_OPT_REDIRECTED_HEADER   4
#define  ND_OPT_MTU                 5

struct NDOptLinkAddrEthernet
{
    u8          type;
    u8          len;
    u8          addr[6];
};

// prefix information
struct NDOptPrefixInfo
{
    u8          type;
    u8          len;
    u8          prefixLen;
    u8          flagsReserved;
    u32         validTime;
    u32         preferredTime;
    u32         reserved2;
    In6Addr     prefix;
};

#define ND_OPT_PI_FLAG_ONLINK       0x80
#define ND_OPT_PI_FLAG_AUTO         0x40

// redirected header
struct NDOptRdHdr
{
    u8          type;
    u8          len;
    u16         reserved1;
    u32         reserved2;
    // followed by IP header and data
};

// MTU option
struct NDOptMtu
{
    u8          type;
    u8          len;
    u16         reserved;
    u32         mtu;
};

// Multicast Listener Discovery Definitions

#define MLD_LISTENER_QUERY          130
#define MLD_LISTENER_REPORT         131
#define MLD_LISTENER_REDUCTION      132

struct ICMP6MldHdr
{
    u8          type;       // type field
    u8          code;       // code field
    u16         cksum;      // checksum field
    u16         maxDelay;
    u16         reserved;
    In6Addr     addr;       // multicast address
};

// ICMPv6 Router Renumbering Definitions

#define ICMP6_ROUTER_RENUMBERING    138   // router renumbering

struct ICMP6RouterRenum
{  // router renumbering header
    u8          type;       // type field
    u8          code;       // code field
    u16         cksum;      // checksum field
    u32         seqnum;
    u8          segnum;
    u8          flags;
    u16         maxdelay;
    u32         reserved;
};

// Router renumbering flags
#define ICMP6_RR_FLAGS_TEST         0x80
#define ICMP6_RR_FLAGS_REQRESULT    0x40
#define ICMP6_RR_FLAGS_FORCEAPPLY   0x20
#define ICMP6_RR_FLAGS_SPECSITE     0x10
#define ICMP6_RR_FLAGS_PREVDONE     0x08

// match prefix part
struct ICMP6RRPcoMatch
{
    u8          code;
    u8          len;
    u8          ordinal;
    u8          matchlen;
    u8          minlen;
    u8          maxlen;
    u16         reserved;
    In6Addr     prefix;
};

// PCO code values
#define RPM_PCO_ADD                 1
#define RPM_PCO_CHANGE              2
#define RPM_PCO_SETGLOBAL           3

// use prefix part
struct ICMP6RRPcoUse
{
    u8          uselen;
    u8          keeplen;
    u8          ramask;
    u8          raflags;
    u32         vltime;
    u32         pltime;
    u32         flags;
    In6Addr     prefix;
};

#define ICMP6_RR_PCOUSE_RAFLAGS_ONLINK      0x20
#define ICMP6_RR_PCOUSE_RAFLAGS_AUTO        0x10

#define ICMP6_RR_PCOUSE_FLAGS_DECRVLTIME    0x80000000
#define ICMP6_RR_PCOUSE_FLAGS_DECRPLTIME    0x40000000

// router renumbering result message
struct ICMP6RRResult
{
    u16         flags;
    u8          ordinal;
    u8          matchedlen;
    u32         ifid;
    In6Addr     prefix;
};

#define ICMP6_RR_RESULT_FLAGS_OOB           0x0002
#define ICMP6_RR_RESULT_FLAGS_FORBIDDEN     0x0001


// RFC 3493

extern const In6Addr In6AddrAny;
extern const In6Addr In6AddrLoopback;
extern const In6Addr In6AddrLinkLocalAllNodes;
extern const In6Addr In6AddrLinkLocalAllRouters;

__inline int IN6_ARE_ADDR_EQUAL(const In6Addr* a, const In6Addr* b)   // RFC 3542
{
    return (memcmp(a->addr, b->addr, 16) == 0) ? true : false;
}

__inline int IN6_IS_ADDR_UNSPECIFIED(const In6Addr* a)
{
    return IN6_ARE_ADDR_EQUAL(a, &In6AddrAny);
}

__inline int IN6_IS_ADDR_LOOPBACK(const In6Addr* a)
{
    return IN6_ARE_ADDR_EQUAL(a, &In6AddrLoopback);
}

__inline int IN6_IS_ADDR_MULTICAST(const In6Addr* a)
{
    return (a->addr[0] == 0xff) ? true : false;
}

__inline int IN6_IS_ADDR_LINKLOCAL(const In6Addr* a)
{
    return (a->addr[0] == 0xfe && (a->addr[1] & 0xc0) == 0x80) ? true : false;
}

__inline int IN6_IS_ADDR_SITELOCAL(const In6Addr* a)
{
    return (a->addr[0] == 0xfe && (a->addr[1] & 0xc0) == 0xc0) ? true : false;
}

__inline int IN6_IS_ADDR_V4MAPPED(const In6Addr* a)
{
    return (memcmp(a->addr, In6AddrAny.addr, 10) == 0 &&
            *(u16*) &a->addr[10] == 0xffff) ? true : false;
}

__inline int IN6_IS_ADDR_V4COMPAT(const In6Addr* a)
{
    return (memcmp(a->addr, In6AddrAny.addr, 12) == 0 &&
            !(memcmp(&a->addr[12], &In6AddrAny.addr[12], 4) == 0 ||
              memcmp(&a->addr[12], &In6AddrLoopback.addr[12], 4) == 0)) ? true : false;
}

__inline int IN6_IS_ADDR_MC_NODELOCAL(const In6Addr* a)
{
    return (IN6_IS_ADDR_MULTICAST(a) && (a->addr[1] & 0xf) == 1) ? true : false;
}

__inline int IN6_IS_ADDR_MC_LINKLOCAL(const In6Addr* a)
{
    return (IN6_IS_ADDR_MULTICAST(a) && (a->addr[1] & 0xf) == 2) ? true : false;
}

__inline int IN6_IS_ADDR_MC_SITELOCAL(const In6Addr* a)
{
    return (IN6_IS_ADDR_MULTICAST(a) && (a->addr[1] & 0xf) == 5) ? true : false;
}

__inline int IN6_IS_ADDR_MC_ORGLOCAL(const In6Addr* a)
{
    return (IN6_IS_ADDR_MULTICAST(a) && (a->addr[1] & 0xf) == 8) ? true : false;
}

__inline int IN6_IS_ADDR_MC_GLOBAL(const In6Addr* a)
{
    return (IN6_IS_ADDR_MULTICAST(a) && (a->addr[1] & 0xf) == 0xe) ? true : false;
}

// Misc.

// IPv6 requires that every link in the internet have an MTU of 1280
// octets or greater. [RFC 2460]
#define IP6_MIN_MTU         1280

#endif  // NINTENDO_ES_NET_INET6_H_INCLUDED
