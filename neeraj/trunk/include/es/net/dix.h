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
