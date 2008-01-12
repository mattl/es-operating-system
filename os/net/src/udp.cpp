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

#include <es/handle.h>
#include <es/net/inet4.h>
#include "udp.h"

s16 UDPReceiver::
checksum(InetMessenger* m)
{
    UDPHdr* udphdr = static_cast<UDPHdr*>(m->fix(sizeof(UDPHdr)));
    s32 sum = m->sumUp(ntohs(udphdr->len));
    Handle<Address> addr;
    addr = m->getRemote();
    sum += addr->sumUp();
    addr = m->getLocal();
    sum += addr->sumUp();
    sum += udphdr->len;
    sum += ntohs(IPPROTO_UDP);
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return ~sum;
}

bool UDPReceiver::
input(InetMessenger* m)
{
    UDPHdr* udphdr = static_cast<UDPHdr*>(m->fix(sizeof(UDPHdr)));
    if (!udphdr)
    {
        return false;   // XXX
    }
    int len = ntohs(udphdr->len);
    if (len < sizeof(UDPHdr) || m->getLength() < len)
    {
        return false;   // XXX
    }

    // Verify the sum
    if (udphdr->sum && checksum(m) != 0)
    {
        return false;
    }

    m->setRemotePort(ntohs(udphdr->src));
    m->setLocalPort(ntohs(udphdr->dst));

    // Separate UDPHdr
    m->movePosition(sizeof(UDPHdr));

    return true;
}

bool UDPReceiver::
output(InetMessenger* m)
{
    long len = sizeof(UDPHdr) + m->getLength();
    if (65535 - sizeof(UDPHdr) < len)
    {
        // XXX Return an error code to the upper layers.
        return false;
    }

    // Add UDPHdr
    m->movePosition(-sizeof(UDPHdr));
    UDPHdr* udphdr = static_cast<UDPHdr*>(m->fix(sizeof(UDPHdr)));
    udphdr->src = htons(m->getLocalPort());
    udphdr->dst = htons(m->getRemotePort());
    udphdr->len = htons(len);   // octets in UDPHdr and user data
    udphdr->sum = 0;
    s16 sum = checksum(m);
    udphdr->sum = (sum == 0) ? 0xffff : sum;
    m->setType(IPPROTO_UDP);

    return true;
}

bool UDPReceiver::
error(InetMessenger* m)
{
    UDPHdr* udphdr = static_cast<UDPHdr*>(m->fix(sizeof(UDPHdr)));

    // Reverse src and dst
    m->setRemotePort(ntohs(udphdr->dst));
    m->setLocalPort(ntohs(udphdr->src));

    return true;
}
