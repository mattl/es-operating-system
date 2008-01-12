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

#ifndef NINTENDO_ES_NET_IGMP_H_INCLUDED
#define NINTENDO_ES_NET_IGMP_H_INCLUDED

#include <es/net/inet4.h>

#define IGMP_VERSION            2   // Default IGMP version to be used

//
// RFC 3376 (IGMPv3)
// RFC 2236 (IGMPv2)
// RFC 1112 (IGMPv1)
//

#define IGMP_V1_ROUTER_PRESENT_TIMEOUT      400     // [sec]
#define IGMP_QUERY_RESPONSE_INTERVAL        100     // 10 seconds
#define IGMP_UNSOLICITED_REPORT_INTERVAL    10      // 10 seconds

// IGMP types
#define IGMP_TYPE_QUERY         0x11
#define IGMP_TYPE_REPORT_V1     0x12
#define IGMP_TYPE_REPORT_V2     0x16
#define IGMP_TYPE_REPORT_V3     0x22
#define IGMP_TYPE_LEAVE         0x17

struct IGMPHdr
{
    u8      type;           // Type
    u8      maxRespTime;    // Max Response Time
    u16     sum;            // Checksum
    InAddr  addr;           // Group Address
};

#endif  // NINTENDO_ES_NET_IGMP_H_INCLUDED
