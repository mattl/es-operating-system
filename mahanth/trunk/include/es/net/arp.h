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

#ifndef NINTENDO_ES_NET_ARP_H_INCLUDED
#define NINTENDO_ES_NET_ARP_H_INCLUDED

#include <es/types.h>
#include <es/net/inet4.h>

//
// Ethernet ARP RFC 826
//

/**
 * This structure is defined for the ARP header.
 */
struct ARPHdr
{
    // Hardware types.
    static const u16 HRD_ETHERNET = 1;          // Ethernet
    static const u16 HRD_LOOPBACK = 772;        // Loopback interface
    static const u16 HRD_IEEE80211 = 801;       // IEEE 802.11
    static const u16 HRD_NONE = 0xFFFE;         // Zero header device

    static const u16 PRO_IP = 0x800;

    // Opcodes.
    static const u16 OP_REQUEST = 1;
    static const u16 OP_REPLY = 2;
    static const u16 OP_RREQUEST = 3;
    static const u16 OP_RREPLY = 4;

    // Constants from RFC 3927
    /** 1 second   (initial random delay)
     */
    static const int PROBE_WAIT = 1;
    /** 3          (number of probe packets)
     */
    static const int PROBE_NUM = 3;
    /** 1 second   (minimum delay till repeated probe)
     */
    static const int PROBE_MIN = 1;
    /** 2 seconds  (maximum delay till repeated probe)
     */
    static const int PROBE_MAX = 2;
    /** 2 seconds  (delay before announcing)
     */
    static const int ANNOUNCE_WAIT = 2;
    /** 2          (number of announcement packets)
     */
    static const int ANNOUNCE_NUM = 2;
    /** 2 seconds  (time between announcement packets)
     */
    static const int ANNOUNCE_INTERVAL = 2;
    /** 10          (max conflicts before rate limiting)
     */
    static const int MAX_CONFLICTS = 10;
    /** 60 seconds  (delay between successive attempts)
     */
    static const int RATE_LIMIT_INTERVAL = 60;
    /** 10 seconds  (minimum interval between defensive ARPs).
     */
    static const int DEFEND_INTERVAL = 10;

    /** hardware address.
     */
    u16     hrd;
    /** protocol address.
     */
    u16     pro;
    /** length of each hardware address.
     */
    u8      hln;
    /** length of each protocol address.
     */
    u8      pln;
    /** opcode.
     */
    u16     op;
    /** hardware address of sender of this packet.
     */
    u8      sha[6];
    /** protocol address of sender of this packet.
     */
    InAddr  spa     __attribute__ ((packed));
    /** hardware address of target of this packet.
     */
    u8      tha[6];
    /** protocol address of target.
     */
    InAddr  tpa     __attribute__ ((packed));
};

#endif  // NINTENDO_ES_NET_ARP_H_INCLUDED
