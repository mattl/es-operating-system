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

#include <stdlib.h>
#include "dix.h"

const u8 DIXInterface::macAllHost[6] = { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x01 };

DIXInterface::DIXInterface(es::NetworkInterface* networkInterface) :
    networkInterface(networkInterface, true),
    stream(this->networkInterface),
    dixReceiver(this),
    inReceiver(this),
    arpReceiver(this),
    NetworkInterface(networkInterface, &dixAccessor, &dixReceiver)
{
    u8 mac[6];
    networkInterface->getMacAddress(mac);
    setMacAddress(mac);
    seed48((u16*) mac);

    inProtocol.setReceiver(&inReceiver);
    arpProtocol.setReceiver(&arpReceiver);

    Conduit::connectBA(&mux, &inProtocol, reinterpret_cast<void*>(DIXHdr::DIX_IP));
    Conduit::connectBA(&mux, &arpProtocol, reinterpret_cast<void*>(DIXHdr::DIX_ARP));
    Conduit::connectBA(&mux, &in6Protocol, reinterpret_cast<void*>(DIXHdr::DIX_IPv6));
}

bool DIXReceiver::input(InetMessenger* m, Conduit* c)
{
    static const u8 bcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    u32 flag;

    // [RFC1122] The link layer MUST include a flag to indicate
    // whether the incoming packet was addressed to a link-layer
    // broadcast address.
    DIXHdr* dixhdr = static_cast<DIXHdr*>(m->fix(sizeof(DIXHdr)));
    if (memcmp(dixhdr->dst, bcast, 6) == 0)
    {
        m->setFlag(InetMessenger::Broadcast);
    }
    else if (dixhdr->dst[0] & 0x01)
    {
        m->setFlag(InetMessenger::Multicast);
    }
    else
    {
        m->setFlag(InetMessenger::Unicast);
    }
    return true;
}

bool DIXReceiver::output(InetMessenger* m, Conduit* c)
{
    static const u8 zero[6] = { 0, 0, 0, 0, 0, 0 };

    DIXHdr* dixhdr = static_cast<DIXHdr*>(m->fix(sizeof(DIXHdr)));
    if (memcmp(zero, dixhdr->dst, 6))
    {
        long len = m->getLength();
        void* packet = m->fix(len);
#ifdef VERBOSE
        esReport("# dix output\n");
        esDump(packet, len);
#endif
        dix->write(packet, len);
    }
    // else discard the packet
    return true;
}

bool DIXInReceiver::output(InetMessenger* m, Conduit* c)
{
    static const u8 zero[6] = { 0, 0, 0, 0, 0, 0 };
    ASSERT(m);

    m->movePosition(-sizeof(DIXHdr));
    DIXHdr* dixhdr = static_cast<DIXHdr*>(m->fix(sizeof(DIXHdr)));
    dix->getMacAddress(dixhdr->src);
    dixhdr->type = htons(DIXHdr::DIX_IP);

    // Fill in dixhdr->dst
    Handle<Inet4Address> nextHop = m->getRemote();
    nextHop->getMacAddress(dixhdr->dst);
    if (memcmp(zero, dixhdr->dst, 6) == 0)
    {
        // Try to resolve the link layer address of nextHop.
        if (nextHop->getScopeID() == 0)
        {
            nextHop->setScopeID(m->getScopeID());
        }
        nextHop->start();
        m->restorePosition();
        nextHop->hold(m);
        return false;
    }
    return true;
}

bool DIXARPReceiver::output(InetMessenger* m, Conduit* c)
{
    static const u8 zero[6] = { 0, 0, 0, 0, 0, 0 };
    ASSERT(m);

    ARPHdr* arphdr = static_cast<ARPHdr*>(m->fix(sizeof(ARPHdr)));
    arphdr->hrd = htons(ARPHdr::HRD_ETHERNET);
    arphdr->pro = htons(ARPHdr::PRO_IP);
    arphdr->hln = 6;
    arphdr->pln = 4;

    m->movePosition(-sizeof(DIXHdr));
    DIXHdr* dixhdr = static_cast<DIXHdr*>(m->fix(sizeof(DIXHdr)));
    dix->getMacAddress(dixhdr->src);
    if (IN_IS_ADDR_LINKLOCAL(arphdr->spa))
    {
        // All ARP packets (*replies* as well as requests) that contain a Link-
        // Local 'sender IP address' MUST be sent using link-layer broadcast
        // instead of link-layer unicast.  This aids timely detection of
        // duplicate addresses. [RFC 3927]
        memset(dixhdr->dst, 0xff, 6);   // bcast
    }
    else if (memcmp(zero, arphdr->tha, 6) && memcmp(dixhdr->src, arphdr->tha, 6))
    {
        memmove(dixhdr->dst, arphdr->tha, 6);
    }
    else
    {
        memset(dixhdr->dst, 0xff, 6);   // bcast
    }
    dixhdr->type = htons(DIXHdr::DIX_ARP);

    return true;
}
