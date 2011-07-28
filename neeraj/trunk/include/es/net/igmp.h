/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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
