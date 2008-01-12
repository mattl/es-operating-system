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

#include "dix.h"

DIXInterface::DIXInterface(IStream* stream) :
    stream(stream),
    dixReceiver(this),
    inReceiver(this),
    arpReceiver(this),
    Interface(stream, &dixAccessor, &dixReceiver)
{
    if (stream)
    {
        Handle<IEthernet> ethernet = stream;
        u8 mac[6];
        ethernet->getMacAddress(mac);
        setMacAddress(mac);
        stream->addRef();
    }

    inProtocol.setReceiver(&inReceiver);
    arpProtocol.setReceiver(&arpReceiver);

    Conduit::connectBA(&mux, &inProtocol, reinterpret_cast<void*>(DIXHdr::DIX_IP));
    Conduit::connectBA(&mux, &arpProtocol, reinterpret_cast<void*>(DIXHdr::DIX_ARP));
    Conduit::connectBA(&mux, &in6Protocol, reinterpret_cast<void*>(DIXHdr::DIX_IPv6));
}

bool DIXReceiver::input(InetMessenger* m)
{
    return true;
}

bool DIXReceiver::output(InetMessenger* m)
{
    static const u8 zero[6] = { 0, 0, 0, 0, 0, 0 };

    DIXHdr* dixhdr = static_cast<DIXHdr*>(m->fix(sizeof(DIXHdr)));
    if (memcmp(zero, dixhdr->dst, 6))
    {
        long len = m->getLength();
        void* packet = m->fix(len);
        esReport("# dix output\n");
        esDump(packet, len);
        dix->write(packet, len);
    }
    // else discard the packet
    return true;
}

bool DIXInReceiver::output(InetMessenger* m)
{
    static const u8 zero[6] = { 0, 0, 0, 0, 0, 0 };
    ASSERT(m);

    m->movePosition(-sizeof(DIXHdr));
    DIXHdr* dixhdr = static_cast<DIXHdr*>(m->fix(sizeof(DIXHdr)));
    dix->getMacAddress(dixhdr->src);
    dixhdr->type = htons(DIXHdr::DIX_IP);

    // Fill in dixhdr->dst
    Address* nextHop = m->getRemote();
    nextHop->getMacAddress(dixhdr->dst);
    if (memcmp(zero, dixhdr->dst, 6) == 0)
    {
        // Try to resolve the link layer address of nextHop.
        nextHop->start();

        // Shold keep m...
    }
    nextHop->release();

    return true;
}

bool DIXARPReceiver::output(InetMessenger* m)
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
