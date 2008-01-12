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

#ifndef NINTENDO_ES_NET_ARP_H_INCLUDED
#define NINTENDO_ES_NET_ARP_H_INCLUDED

#include <es/types.h>
#include <es/net/inet4.h>

//
// Ethernet ARP RFC 826
//

struct ARPHdr
{
    static const u16 HRD_ETHERNET = 1;          // Ethernet
    static const u16 HRD_LOOPBACK = 772;        // Loopback interface
    static const u16 HRD_IEEE80211 = 801;       // IEEE 802.11
    static const u16 HRD_NONE = 0xFFFE;         // Zero header device

    static const u16 PRO_IP = 0x800;

    static const u16 OP_REQUEST = 1;
    static const u16 OP_REPLY = 2;
    static const u16 OP_RREQUEST = 3;
    static const u16 OP_RREPLY = 4;

    // Constants from RFC 3927
    static const int PROBE_WAIT = 1;            // second   (initial random delay)
    static const int PROBE_NUM = 3;             //          (number of probe packets)
    static const int PROBE_MIN = 1;             // second   (minimum delay till repeated probe)
    static const int PROBE_MAX = 2;             // seconds  (maximum delay till repeated probe)
    static const int ANNOUNCE_WAIT = 2;         // seconds  (delay before announcing)
    static const int ANNOUNCE_NUM = 2;          //          (number of announcement packets)
    static const int ANNOUNCE_INTERVAL = 2;     // seconds  (time between announcement packets)
    static const int MAX_CONFLICTS = 10;        //          (max conflicts before rate limiting)
    static const int RATE_LIMIT_INTERVAL = 60;  // seconds  (delay between successive attempts)
    static const int DEFEND_INTERVAL = 10;      // seconds  (minimum interval between defensive

    u16     hrd;
    u16     pro;
    u8      hln;
    u8      pln;
    u16     op;
    u8      sha[6];
    InAddr  spa     __attribute__ ((packed));
    u8      tha[6];
    InAddr  tpa     __attribute__ ((packed));
};

#endif  // NINTENDO_ES_NET_ARP_H_INCLUDED
