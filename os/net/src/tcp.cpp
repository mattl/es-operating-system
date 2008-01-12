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
#include "tcp.h"

s16 TCPReceiver::
checksum(InetMessenger* m)
{
    int len = m->getLength();
    s32 sum = m->sumUp(len);
    Handle<Address> addr;
    addr = m->getRemote();
    sum += addr->sumUp();
    addr = m->getLocal();
    sum += addr->sumUp();
    sum += len;
    sum += ntohs(IPPROTO_TCP);
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return ~sum;
}

bool TCPReceiver::input(InetMessenger* m)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    if (!tcphdr)
    {
        return false;   // XXX
    }
    int hlen = tcphdr->getHdrSize();
    if (hlen < sizeof(TCPHdr) || m->getLength() < hlen)
    {
        return false;   // XXX
    }

    // Verify the sum
    if (checksum(m) != 0)
    {
        return false;
    }

    m->setRemotePort(ntohs(tcphdr->src));
    m->setLocalPort(ntohs(tcphdr->dst));

    return true;
}

bool TCPReceiver::output(InetMessenger* m)
{
    // Update TCPHdr
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    tcphdr->src = htons(m->getLocalPort());
    tcphdr->dst = htons(m->getRemotePort());
    tcphdr->sum = 0;
    tcphdr->sum = checksum(m);
    m->setType(IPPROTO_TCP);

    return true;
}

bool TCPReceiver::error(InetMessenger* m)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));

    // Reverse src and dst
    m->setRemotePort(ntohs(tcphdr->dst));
    m->setLocalPort(ntohs(tcphdr->src));

    return true;
}
