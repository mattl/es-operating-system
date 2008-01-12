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

#ifndef NINTENDO_ES_NET_IGMP_H_INCLUDED
#define NINTENDO_ES_NET_IGMP_H_INCLUDED

#include <es/net/inet4.h>

// RFC 1112 (IGMPv1)
// RFC 2236 (IGMPv2)
// RFC 3376 (IGMPv3)

struct IGMPHdr
{
    u8      type;           // Type
    u8      maxRespTime;    // Max response time in units of 1/10 second
    u16     sum;            // Checksum
    InAddr  addr;           // Group address

    // IGMP types
    static const u8 Query = 0x11;
    static const u8 ReportVer1 = 0x12;
    static const u8 ReportVer2 = 0x16;
    static const u8 ReportVer3 = 0x22;
    static const u8 Leave = 0x17;

    // Defaults
    static const int QueryResponseInterval = 100;       // [in 1/10 second]
    static const int UnsolicitedReportInterval = 10;    // [second]
    static const int Ver1RouterPresentTimeout = 400;    // [sec]
};

#endif  // NINTENDO_ES_NET_IGMP_H_INCLUDED
