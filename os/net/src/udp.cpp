/*
 * Copyright (c) 2006, 2007
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
#include <es/net/icmp.h>
#include "inet4address.h"
#include "socket.h"
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
input(InetMessenger* m, Conduit* c)
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

    Handle<Inet4Address> address = m->getLocal();
    if (address->isMulticast())
    {
        Collection<Socket*>::Iterator iter(address->sockets.begin());
        while (iter.hasNext())
        {
            Socket* socket = iter.next();
            if (m->getLocalPort() == socket->getLocalPort())
            {
                Handle<Inet4Address> address = socket->getLocal();
                m->setLocal(address);
                Visitor v(m);
                c->accept(&v);
            }
        }
    }
    m->setLocal(address);

    return true;
}

bool UDPReceiver::
output(InetMessenger* m, Conduit* c)
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
error(InetMessenger* m, Conduit* c)
{
    UDPHdr* udphdr = static_cast<UDPHdr*>(m->fix(sizeof(UDPHdr)));

    // Reverse src and dst
    m->setRemotePort(ntohs(udphdr->dst));
    m->setLocalPort(ntohs(udphdr->src));

    return true;
}

bool UDPUnreachReceiver::
input(InetMessenger* m, Conduit* c)
{
    // Now we need to access the original IP header.
    m->restorePosition();
    IPHdr* iphdr = static_cast<IPHdr*>(m->fix(sizeof(IPHdr)));

    // [RFC 1122]
    //
    //   An ICMP error message MUST NOT be sent as the result of
    //   receiving:
    //
    //   *    an ICMP error message, or
    //   *    a datagram destined to an IP broadcast or IP multicast
    //        address, or
    //   *    a datagram sent as a link-layer broadcast, or
    //   *    a non-initial fragment, or
    //   *    a datagram whose source address does not define a single
    //        host -- e.g., a zero address, a loopback address, a
    //        broadcast address, a multicast address, or a Class E
    //        address.
    if (!IN_ARE_ADDR_EQUAL(iphdr->dst, InAddrBroadcast) &&
        !IN_IS_ADDR_MULTICAST(iphdr->dst) &&
        !IN_IS_ADDR_UNSPECIFIED(iphdr->src) &&
        !IN_IS_ADDR_LOOPBACK(iphdr->src) &&
        !IN_ARE_ADDR_EQUAL(iphdr->src, InAddrBroadcast) &&
        !IN_IS_ADDR_MULTICAST(iphdr->src))  // XXX check for directed broadcast
    {
        int len = iphdr->getHdrSize() + 8;
        int pos = 14 + 60 + sizeof(ICMPUnreach);    // XXX Assume MAC, IPv4
        Handle<InetMessenger> r = new InetMessenger(&InetReceiver::output, pos + len, pos);

        memmove(r->fix(len), iphdr, len);
        r->setRemote(m->getRemote());
        r->setLocal(m->getLocal());
        Visitor v(r);
        unreachProtocol->accept(&v, unreachProtocol->getB());
    }

    return true;
}
