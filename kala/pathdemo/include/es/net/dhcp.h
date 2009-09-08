/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_NET_DHCP_H_INCLUDED
#define NINTENDO_ES_NET_DHCP_H_INCLUDED

#include <es/net/inet4.h>

// cf. RFC 2131, RFC 2132.

struct DHCPOption
{
    // Pad Option
    //
    //  Code
    // +-----+
    // |  0  |
    // +-----+
    static const u8 Pad = 0;

    // Subnet Mask
    //
    //  Code   Len        Subnet Mask
    // +-----+-----+-----+-----+-----+-----+
    // |  1  |  4  |  m1 |  m2 |  m3 |  m4 |
    // +-----+-----+-----+-----+-----+-----+
    static const u8 SubnetMask = 1;

    // Time Offset
    //
    //  Code   Len        Time Offset
    // +-----+-----+-----+-----+-----+-----+
    // |  2  |  4  |  n1 |  n2 |  n3 |  n4 |
    // +-----+-----+-----+-----+-----+-----+
    static const u8 TimeOffset = 2;

    // Router Option
    //
    //  Code   Len         Address 1               Address 2
    // +-----+-----+-----+-----+-----+-----+-----+-----+--
    // |  3  |  n  |  a1 |  a2 |  a3 |  a4 |  a1 |  a2 |  ...
    // +-----+-----+-----+-----+-----+-----+-----+-----+--
    static const u8 Router = 3;

    // Domain Name Server Option
    //
    //  Code   Len         Address 1               Address 2
    // +-----+-----+-----+-----+-----+-----+-----+-----+--
    // |  6  |  n  |  a1 |  a2 |  a3 |  a4 |  a1 |  a2 |  ...
    // +-----+-----+-----+-----+-----+-----+-----+-----+--
    static const u8 DomainNameServer = 6;

    // Host Name Option
    //
    //  Code   Len                 Host Name
    // +-----+-----+-----+-----+-----+-----+-----+-----+--
    // |  12 |  n  |  h1 |  h2 |  h3 |  h4 |  h5 |  h6 |  ...
    // +-----+-----+-----+-----+-----+-----+-----+-----+--
    static const u8 HostName = 12;

    // Domain Name
    //
    //  Code   Len        Domain Name
    // +-----+-----+-----+-----+-----+-----+--
    // |  15 |  n  |  d1 |  d2 |  d3 |  d4 |  ...
    // +-----+-----+-----+-----+-----+-----+--
    static const u8 DomainName = 15;

    // Default IP Time-to-live
    //
    //  Code   Len   TTL
    // +-----+-----+-----+
    // |  23 |  1  | ttl |
    // +-----+-----+-----+
    static const u8 DefaultTTL = 23;

    // Interface MTU Option
    //
    //  Code   Len      MTU
    // +-----+-----+-----+-----+
    // |  26 |  2  |  m1 |  m2 |
    // +-----+-----+-----+-----+
    static const u8 InterfaceMTU = 26;

    // Broadcast Address Option
    //
    //  Code   Len     Broadcast Address
    // +-----+-----+-----+-----+-----+-----+
    // |  28 |  4  |  b1 |  b2 |  b3 |  b4 |
    // +-----+-----+-----+-----+-----+-----+
    static const u8 BroadcastAddress = 28;

    // Static Route Option
    //
    //  Code   Len         Destination 1           Router 1
    // +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
    // |  33 |  n  |  d1 |  d2 |  d3 |  d4 |  r1 |  r2 |  r3 |  r4 |
    // +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
    //         Destination 2           Router 2
    // +-----+-----+-----+-----+-----+-----+-----+-----+---
    // |  d1 |  d2 |  d3 |  d4 |  r1 |  r2 |  r3 |  r4 | ...
    // +-----+-----+-----+-----+-----+-----+-----+-----+---
    static const u8 StaticRoute = 33;

    // ARP Cache Timeout Option
    //
    //  Code   Len           Time
    // +-----+-----+-----+-----+-----+-----+
    // |  35 |  4  |  t1 |  t2 |  t3 |  t4 |
    // +-----+-----+-----+-----+-----+-----+
    static const u8 ARPCacheTimeout = 35;

    //
    // DHCP Extensions
    //

    // Requested IP Address
    //
    //  Code   Len          Address
    // +-----+-----+-----+-----+-----+-----+
    // |  50 |  4  |  a1 |  a2 |  a3 |  a4 |
    // +-----+-----+-----+-----+-----+-----+
    static const u8 RequestedAddress = 50;

    // IP Address Lease Time
    //
    //  Code   Len         Lease Time
    // +-----+-----+-----+-----+-----+-----+
    // |  51 |  4  |  t1 |  t2 |  t3 |  t4 |
    // +-----+-----+-----+-----+-----+-----+
    static const u8 LeaseTime = 51;

    // Option Overload
    //
    //  Code   Len  Value
    // +-----+-----+-----+
    // |  52 |  1  |1/2/3|
    // +-----+-----+-----+
    static const u8 Overload = 52;

    // DHCP message type
    //
    //  Code   Len  Type
    // +-----+-----+-----+
    // |  53 |  1  | 1-7 |
    // +-----+-----+-----+
    static const u8 Type = 53;

    // Server Identifier
    //
    // Code   Len            Address
    // +-----+-----+-----+-----+-----+-----+
    // |  54 |  4  |  a1 |  a2 |  a3 |  a4 |
    // +-----+-----+-----+-----+-----+-----+
    static const u8 ServerID = 54;

    // Parameter Request List
    //
    //  Code   Len   Option Codes
    // +-----+-----+-----+-----+---
    // |  55 |  n  |  c1 |  c2 | ...
    // +-----+-----+-----+-----+---
    static const u8 RequestList = 55;

    // Maximum DHCP Message Size
    //
    //  Code   Len     Length
    // +-----+-----+-----+-----+
    // |  57 |  2  |  l1 |  l2 |
    // +-----+-----+-----+-----+
    static const u8 MaxSize = 57;

    // Renewal (T1) Time Value
    //
    //  Code   Len         T1 Interval
    // +-----+-----+-----+-----+-----+-----+
    // |  58 |  4  |  t1 |  t2 |  t3 |  t4 |
    // +-----+-----+-----+-----+-----+-----+
    static const u8 RenewalTime = 58;

    // Rebinding (T2) Time Value
    //
    //  Code   Len         T2 Interval
    // +-----+-----+-----+-----+-----+-----+
    // |  59 |  4  |  t1 |  t2 |  t3 |  t4 |
    // +-----+-----+-----+-----+-----+-----+
    static const u8 RebindingTime = 59;

    // Client-identifier
    //
    // Code   Len   Type  Client-Identifier
    // +-----+-----+-----+-----+-----+---
    // |  61 |  n  |  t1 |  i1 |  i2 | ...
    // +-----+-----+-----+-----+-----+---
    static const u8 ClientIdentifier = 61;

    // End option - The last option must always be the 'end' option.
    //
    //  Code
    // +-----+
    // | 255 |
    // +-----+
    static const u8 End = 255u;
};

// DHCP Messeage Types
struct DHCPType
{
    static const u8 Discover = 1;
    static const u8 Offer = 2;
    static const u8 Request = 3;
    static const u8 Decline = 4;
    static const u8 Ack = 5;
    static const u8 Nak = 6;
    static const u8 Release = 7;
};

struct DHCPState
{
    enum
    {
        Init,
        Selecting,
        Requesting,
        Bound,
        Renewing,
        Rebinding,
        InitReboot,
        Rebooting
    };
};

struct DHCPHdr
{
    u8      op;             // Opcode (1=request, 2=reply)
    u8      htype;          // Hardware type (1=Ethernet)
    u8      hlen;           // Hardware address length (6 for Ethernet)
    u8      hops;           // Hop count
    u32     xid;            // Transaction ID
    u16     secs;           // Number of seconds
    u16     flags;          // DHCP_BROADCAST or zero
    InAddr  ciaddr;         // Client IP address
    InAddr  yiaddr;         // your IP address
    InAddr  siaddr;         // Server IP address
    InAddr  giaddr;         // Gateway IP address
    u8      chaddr[16];     // Client hardware address
    u8      sname[64];      // Server host name
    u8      file[128];      // Boot filename
    InAddr  cookie;         // Magic Cookie
    // Options...

    static const int ServerPort = 67;
    static const int ClientPort = 68;

    static const int MinHdrSize = 236;
    static const int BootpHdrSize = 300;    // BOOTP message length

    // Opcode
    static const u8 Request = 1;
    static const u8 Reply = 2;

    // Flags
    static const u16 Broadcast = 0x8000u;

    // Overload Types
    static const u8 File = 1;
    static const u8 Sname = 2;
    static const u8 FileAndSname = 3;

    static const InAddr magicCookie;        // 99.130.83.99
};

#endif  // NINTENDO_ES_NET_DHCP_H_INCLUDED
