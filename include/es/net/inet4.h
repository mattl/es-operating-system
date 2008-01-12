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

#ifndef NINTENDO_ES_NET_INET4_H_INCLUDED
#define NINTENDO_ES_NET_INET4_H_INCLUDED

#include <es/types.h>

#define INET_ADDRSTRLEN     16

#define IPPROTO_ICMP        1
#define IPPROTO_IGMP        2
#define IPPROTO_TCP         6
#define IPPROTO_UDP         17

#define AF_UNSPEC           0       // Unspecified
#define AF_INET             2       // ARPA Internet protocols
#define AF_ARP              28      // ARP (RFC 826)

#define PF_UNSPEC           AF_UNSPEC
#define PF_INET             AF_INET
#define PF_ARP              AF_ARP

#define INADDR_ANY              ((u32) 0x00000000)  // 0.0.0.0
#define INADDR_BROADCAST        ((u32) 0xffffffff)  // 255.255.255.255
#define INADDR_LOOPBACK         ((u32) 0x7f000001)  // 127.0.0.1
#define INADDR_UNSPEC_GROUP     ((u32) 0xe0000000)  // 224.0.0.0
#define INADDR_ALLHOSTS_GROUP   ((u32) 0xe0000001)  // 224.0.0.1
#define INADDR_MAX_LOCAL_GROUP  ((u32) 0xe00000ff)  // 224.0.0.255

struct InAddr
{
    u32     addr;
};

inline int operator==(const InAddr& a1, const InAddr& a2)
{
    return a1.addr == a2.addr;
}

inline int operator!=(const InAddr& a1, const InAddr& a2)
{
    return a1.addr != a2.addr;
}

inline int operator<(const InAddr& a1, const InAddr& a2)
{
    return a1.addr < a2.addr;
}

struct SockAddrIn
{
    u8      len;            // size of socket address structure
    u8      family;         // the address family
    u16     port;           // the port number
    InAddr  addr;           // the Internet address
};

struct IPMreq
{
   InAddr   multiaddr;      // IP address of group
   InAddr   interface;      // IP address of interface
};

//
// IP RFC 791
//

struct IPHdr
{
    u8      verlen;         // Version and header length
    u8      tos;            // Type of service
    u16     len;            // packet length
    u16     id;             // Identification
    u16     frag;           // Fragment information
    u8      ttl;            // Time to live
    u8      proto;          // Protocol
    u16     sum;            // Header checksum
    InAddr  src;            // IP source
    InAddr  dst;            // IP destination

    int getVersion() const
    {
        return (verlen >> 4);
    }

    int getHdrSize() const
    {
        return ((verlen & 0x0f) << 2);
    }

    int getSize() const
    {
        return len;
    }
};

#define IP_VER              0x4     // IP version 4
#define IP_VERHLEN          0x45    // Default 20 bytes header

// Fragment information
#define IP_DF               0x4000  // Don't fragment
#define IP_MF               0x2000  // More fragments
#define IP_FO               0x1fff  // Fragment offset

#define IP_FRAG(ip)         (((ip)->frag & IP_FO) << 3)

#define IP_MIN_HLEN         20
#define IP_MAX_HLEN         60

// Options
#define IPOPT_EOOL          0x00    // End of Option List
#define IPOPT_NOP           0x01    // No Operation
#define IPOPT_SEC           0x82    // Security
#define IPOPT_LSRR          0x83    // Loose Source and Record Route
#define IPOPT_SSRR          0x89    // Strict Source and Record Route
#define IPOPT_RR            0x07    // Record Route
#define IPOPT_TS            0x44    // Internet Timestamp

extern const InAddr InAddrAny;
extern const InAddr InAddrLoopback;
extern const InAddr InBroadcast;

__inline int IN_ARE_ADDR_EQUAL(InAddr a, InAddr b)
{
    return a == b;
}

__inline int IN_IS_ADDR_UNSPECIFIED(InAddr a)
{
    return a == InAddrAny;
}

__inline int IN_IS_ADDR_LOOPBACK(InAddr a)
{
    u8* bytes = (u8*) &a.addr;
    return bytes[0] == 127;
}

__inline int IN_IS_ADDR_MULTICAST(InAddr a)
{
    u8* bytes = (u8*) &a.addr;
    return (bytes[0] & 0xf0) == 0xe0;   // class D address?
}

__inline int IN_IS_ADDR_RESERVED(InAddr a)
{
    u8* bytes = (u8*) &a.addr;
    return (bytes[0] & 0xf8) == 0xf0;   // class E address?
}

__inline int IN_IS_ADDR_LINKLOCAL(InAddr a)
{
    u8* bytes = (u8*) &a.addr;
    return (bytes[0] == 169) && (bytes[1] == 254);
}

__inline int IN_IS_ADDR_IN_NET(InAddr a, InAddr n, InAddr m)
{
    return ((a.addr & m.addr) == (n.addr & m.addr));
}

// Misc.

#define IP_MIN_MTU          576

#endif  // NINTENDO_ES_NET_INET4_H_INCLUDED
