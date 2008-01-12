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

#ifndef NINTENDO_ES_NET_DIX_H_INCLUDED
#define NINTENDO_ES_NET_DIX_H_INCLUDED

#include <es/types.h>

//
// Ethernet
//

struct DIXHdr
{
    // Ethernet type
    static const u16 DIX_IP = 0x0800;               // DOD Internet Protocol (IP)
    static const u16 DIX_ARP = 0x0806;              // Address Resolution Protocol (ARP)
    static const u16 DIX_RARP = 0x8035;             // Reverse Address Resolution Protocol (RARP)
    static const u16 DIX_IPv6 = 0x86dd;             // Internet Protocol, Version 6 (IPv6)
    static const u16 DIX_PAUSE = 0x8808;            // IEEE 802.3x PAUSE Frame (01:80:C2:00:00:01)
    static const u16 DIX_PPPoE_DISCOVERY = 0x8863;  // PPPoE Discovery Stage
    static const u16 DIX_PPPoE_SESSION = 0x8864;    // PPPoE Session Stage

    static const int ALEN = 6;                      // Ethernet address length

    u8  dst[ALEN];
    u8  src[ALEN];
    u16 type;
};

#endif  // NINTENDO_ES_NET_DIX_H_INCLUDED
