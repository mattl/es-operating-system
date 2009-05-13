/*
 * Copyright 2008, 2009 Google Inc.
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

#include <string.h>
#include <es.h>
#include <es/endian.h>
#include <es/handle.h>
#include <es/device/INetworkInterface.h>
#include <es/base/IStream.h>
#include <es/net/arp.h>
#include <es/net/dix.h>
#include <es/net/inet4.h>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

struct ArpFrame
{
    DIXHdr  dix;
    ARPHdr  arp;
};

static u8 buf[1514];

int main()
{
    Object* root = 0;
    esInit(&root);

    Handle<es::Context> context = root;
    TEST(context);

    Handle<es::Stream> stream(context->lookup("device/ethernet"));
    TEST(stream);
    Handle<es::NetworkInterface> nic = stream;
    TEST(nic);

    u8 mac[6];
    nic->getMacAddress(mac);
    esReport("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    nic->start();
    nic->setPromiscuousMode(false);

    InAddr addr = { htonl(169 << 24 | 254 << 16 | 0 << 8 | 1) };

    ArpFrame probe;
    memset(probe.dix.dst, 0xff, 6);
    memcpy(probe.dix.src, mac, 6);
    probe.dix.type = htons(DIXHdr::DIX_ARP);
    probe.arp.hrd = htons(ARPHdr::HRD_ETHERNET);
    probe.arp.pro = htons(ARPHdr::PRO_IP);
    probe.arp.hln = 6;
    probe.arp.pln = 4;
    probe.arp.op = htons(ARPHdr::OP_REQUEST);
    memcpy(probe.arp.sha, mac, 6);
    probe.arp.spa.addr = htonl(INADDR_ANY);
    memset(probe.arp.tha, 0x00, 6);
    probe.arp.tpa = addr;

    // Send probes
    for (int i = 0; i < 10; ++i)
    {
        esReport("link: %d\n", nic->getLinkState());
        int len = stream->write(&probe, sizeof probe);
        TEST(len == sizeof probe);
        esReport("Sent %d bytes.\n", len);

        esSleep(10000000);
    }

    es::NetworkInterface::Statistics stat;
    nic->getStatistics(&stat);
    esReport("outOctets: %llu\n", stat.outOctets);
    esReport("outUcastPkts: %u\n", stat.outUcastPkts);
    esReport("outNUcastPkts: %u\n", stat.outNUcastPkts);

    esReport("done.\n");

    // Receive packets
    for (int i = 0; i < 4; ++i)
    {
        int len = stream->read(buf, sizeof(buf));
        if (0 < len)
        {
            esReport("# input\n");
            esDump(buf, len);
        }
    }

    nic->getStatistics(&stat);
    esReport("inOctets: %llu\n", stat.inOctets);
    esReport("inUcastPkts: %u\n", stat.inUcastPkts);
    esReport("inNUcastPkts: %u\n", stat.inNUcastPkts);
    esReport("inErrors: %u\n", stat.inErrors);

    nic->stop();

    return 0;
}
