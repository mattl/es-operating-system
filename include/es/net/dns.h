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

#ifndef NINTENDO_ES_NET_DNS_H_INCLUDED
#define NINTENDO_ES_NET_DNS_H_INCLUDED

#include <es/endian.h>
#include <es/types.h>

//
// Domain Name [RFC 1035]
//

struct DNSHdr
{
    u16 id;         // identifier
    u16 flags;      // QR | Opcode:4 | AA | TC | RD | RA | Z | AD | CD | Rcode:4
    u16 qdcount;    // # of questions
    u16 ancount;    // # of answer RRs
    u16 nscount;    // # of authority RRs
    u16 arcount;    // # of additional RRs

    static const int Port = 53;

    static const int LabelMax = 63;
    static const int NameMax = 255;
    static const int UDPMax = 512;

    // QR
    static const u16 Query = 0x0000;
    static const u16 Response = 0x8000;

    // Opcode
    static const u16 StandardQuery = 0x0000;
    static const u16 InverseQuery = 0x0800;
    static const u16 ServerStatus = 0x1000;
    static const u16 Notify = 0x2000;       // [RFC 1996]
    static const u16 Update = 0x2800;       // [RFC 2136]

    static const u16 AA = 0x0400;           // authoritative answer
    static const u16 TC = 0x0200;           // truncation
    static const u16 RD = 0x0100;           // recursion desired
    static const u16 RA = 0x0080;           // recursion available

    // Rcode
    static const u16 NoError = 0x0000;
    static const u16 FormatError = 0x0001;
    static const u16 ServerFailure = 0x0002;
    static const u16 NameError = 0x0003;
    static const u16 NotImplemented = 0x0004;
    static const u16 Refused = 0x0005;

    u16 getID() const
    {
        return ntohs(id);
    }

    bool isResponse() const
    {
        return ntohs(flags) & Response;
    }

    bool isQuery() const
    {
        return !isResponse();
    }

    u16 getResponseCode() const
    {
        return ntohs(flags) & 0x0f;
    }

    void setResponseCode(u16 code)
    {
        code &= 0x0f;
        code |= ntohs(flags) & ~0x0f;
        flags = htons(code);
    }
};

struct DNSType
{
    enum
    {
        A = 1,          // a host address
        NS = 2,         // an authoritative name server
        MD = 3,         // a mail destination (Obsolete - use MX)
        MF = 4,         // a mail forwarder (Obsolete - use MX)
        CNAME = 5,      // the canonical name for an alias
        SOA = 6,        // marks the start of a zone of authority
        MB = 7,         // a mailbox domain name (EXPERIMENTAL)
        MG = 8,         // a mail group member (EXPERIMENTAL)
        MR = 9,         // a mail rename domain name (EXPERIMENTAL)
//      NULL = 10,      // a null RR (EXPERIMENTAL)
        WKS = 11,       // a well known service description
        PTR = 12,       // a domain name pointer
        HINFO = 13,     // host information
        MINFO = 14,     // mailbox or mail list information
        MX = 15,        // mail exchange
        TXT = 16,       // text strings
        RP = 17,        // responsible Person  [RFC 1183]
        AFSDB = 18,     // AFS Data Base location  [RFC 1183]
        X25 = 19,       // X.25 PSDN address  [RFC 1183]
        ISDN = 20,      // ISDN address  [RFC 1183]
        RT = 21,        // route Through  [RFC 1183]
        NSAP = 22,      // NSAP address, NSAP style A record  [RFC 1706]
        NSAP_PTR = 23,  //
        SIG = 24,       // security signature  [RFC 2535], [RFC 2931]
        KEY = 25,       // security key  [RFC 2535]
        PX = 26,        // X.400 mail mapping information  [RFC 2163]
        GPOS = 27,      // geographical Position  [RFC 1712]
        AAAA = 28,      // IPv6 Address  [RFC 1886]
        LOC = 29,       // location Information  [RFC 1876]
        NXT = 30,       // next Domain  [RFC 2535]
        EID = 31,       // endpoint Identifier
        NIMLOC = 32,    // nimrod Locator
        NB = 32,        // NetBIOS general Name Service
        SRV = 33,       // server Selection
        NBSTAT = 33,    // NetBIOS NODE STATUS  [RFC 2052], [RFC 2782]
        ATMA = 34,      // ATM Address
        NAPTR = 35,     // naming authority pointer  [RFC 2168], [RFC 2915]
        KX = 36,        // key Exchanger  [RFC 2230]
        CERT = 37,      // [RFC 2538]
        A6 = 38,        // [RFC 2874]
        DNAME = 39,     // [RFC 2672]
        SINK = 40,      //
        OPT = 41,       // [RFC 2671]
        APL = 42        // [RFC 3123]
    };
};

struct DNSClass
{
    enum
    {
        IN = 1,     // the Internet
        CS = 2,     // the CSNET class (Obsolete)
        CH = 3,     // the CHAOS class
        HS = 4      // Hesiod [Dyer 87]
    };
};

// RRs format
//                                     1  1  1  1  1  1
//       0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//     |                                               |
//     /                                               /
//     /                      NAME                     /
//     |                                               |
//     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//     |                      TYPE                     |
//     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//     |                     CLASS                     |
//     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//     |                      TTL                      |
//     |                                               |
//     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//     |                   RDLENGTH                    |
//     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
//     /                     RDATA                     /
//     /                                               /
//     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
struct DNSRR
{
    static const int Size = 10;

    u16 type;
    u16 cls;
    u32 ttl;
    u16 rdlength;
};

#endif  // NINTENDO_ES_NET_DNS_H_INCLUDED
