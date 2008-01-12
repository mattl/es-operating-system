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

#ifndef NINTENDO_ES_NET_INET4_H_INCLUDED
#define NINTENDO_ES_NET_INET4_H_INCLUDED

#include <es.h>
#include <es/endian.h>
#include <es/types.h>

static const int INET_ADDRSTRLEN = 16;

static const int IPPROTO_ICMP = 1;
static const int IPPROTO_IGMP = 2;
static const int IPPROTO_TCP = 6;
static const int IPPROTO_UDP = 17;

static const int AF_UNSPEC = 0;     // Unspecified
static const int AF_INET = 2;       // ARPA Internet protocols
static const int AF_ARP = 28;       // ARP (RFC 826)

static const int PF_UNSPEC = AF_UNSPEC;
static const int PF_INET = AF_INET;
static const int PF_ARP = AF_ARP;

static const u32 INADDR_ANY = 0x00000000;               // 0.0.0.0
static const u32 INADDR_BROADCAST = 0xffffffff;         // 255.255.255.255
static const u32 INADDR_LOOPBACK = 0x7f000001;          // 127.0.0.1
static const u32 INADDR_UNSPEC_GROUP = 0xe0000000;      // 224.0.0.0
static const u32 INADDR_ALLHOSTS_GROUP = 0xe0000001;    // 224.0.0.1
static const u32 INADDR_ALLROUTERS_GROUP = 0xe0000002;  // 224.0.0.2
static const u32 INADDR_MAX_LOCAL_GROUP = 0xe00000ff;   // 224.0.0.255

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
    static const int MinHdrSize = 20;
    static const int MaxHdrSize = 60;

    static const u16 DontFragment = 0x4000;
    static const u16 MoreFragments = 0x2000;
    static const u16 FragmentOffset = 0x1fff;

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

    void setVersion()
    {
        verlen &= 0x0f;
        verlen |= 0x40;
    }

    int getHdrSize() const
    {
        return ((verlen & 0x0f) << 2);
    }

    void setHdrSize(int hlen)
    {
        ASSERT(MinHdrSize <= hlen && hlen <= MaxHdrSize && ((hlen & 3) == 0));
        verlen &= ~0x0f;
        verlen |= hlen >> 2;
    }

    int getSize() const
    {
        return ntohs(len);
    }

    void setSize(int len)
    {
        this->len = htons(len);
    }

    int getOffset() const
    {
        int offset = ntohs(frag);
        return (offset & FragmentOffset) << 3;
    }

    void setOffset(u16 offset)
    {
        ASSERT((offset & 7) == 0 && offset <= 65512);
        offset >>= 3;
        offset |= (ntohs(frag) & FragmentOffset);
        frag = htons(offset);
    }

    bool moreFragments()
    {
        return ntohs(frag) & MoreFragments;
    }

    bool dontFragment()
    {
        return ntohs(frag) & DontFragment;
    }

    u16 getId()
    {
        return ntohs(id);
    }
};

// Options
static const int IPOPT_COPIED = 0x80;   // Copied flag
static const int IPOPT_EOOL = 0x00;     // End of Option List
static const int IPOPT_NOP = 0x01;      // No Operation
static const int IPOPT_SEC = 0x82;      // Security
static const int IPOPT_LSRR = 0x83;     // Loose Source and Record Route
static const int IPOPT_SSRR = 0x89;     // Strict Source and Record Route
static const int IPOPT_RR = 0x07;       // Record Route
static const int IPOPT_TS = 0x44;       // Internet Timestamp

extern const InAddr InAddrAny;
extern const InAddr InAddrLoopback;
extern const InAddr InAddrBroadcast;
extern const InAddr InAddrAllHosts;
extern const InAddr InAddrAllRouters;

inline int IN_ARE_ADDR_EQUAL(InAddr a, InAddr b)
{
    return a == b;
}

inline int IN_IS_ADDR_UNSPECIFIED(InAddr a)
{
    return a == InAddrAny;
}

inline int IN_IS_ADDR_LOOPBACK(InAddr a)
{
    u8* bytes = (u8*) &a.addr;
    return bytes[0] == 127;
}

inline int IN_IS_ADDR_MULTICAST(InAddr a)
{
    u8* bytes = (u8*) &a.addr;
    return (bytes[0] & 0xf0) == 0xe0;   // class D address?
}

inline int IN_IS_ADDR_RESERVED(InAddr a)
{
    u8* bytes = (u8*) &a.addr;
    return (bytes[0] & 0xf8) == 0xf0;   // class E address?
}

inline int IN_IS_ADDR_LINKLOCAL(InAddr a)
{
    u8* bytes = (u8*) &a.addr;
    return (bytes[0] == 169) && (bytes[1] == 254);
}

inline int IN_IS_ADDR_IN_NET(InAddr a, InAddr n, InAddr m)
{
    return ((a.addr & m.addr) == (n.addr & m.addr));
}

// Misc.

static const int IP_MIN_MTU = 576;

#endif  // NINTENDO_ES_NET_INET4_H_INCLUDED
