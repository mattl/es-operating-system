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

#include "icmp4.h"

//
// ICMPReceiver
//

s16 ICMPReceiver::checksum(InetMessenger* m)
{
    int len = m->getLength();
    ICMPHdr* icmphdr = static_cast<ICMPHdr*>(m->fix(len));
    s32 sum = m->sumUp(len);
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return ~sum;
}

bool ICMPReceiver::input(InetMessenger* m)
{
    int len = m->getLength();
    if (len < sizeof(ICMPHdr) || checksum(m) != 0)
    {
        return false;       // XXX
    }
    return true;
}

bool ICMPReceiver::output(InetMessenger* m)
{
    ICMPHdr* icmphdr = static_cast<ICMPHdr*>(m->fix(sizeof(ICMPHdr)));
    icmphdr->sum = 0;
    icmphdr->sum = checksum(m);
    return true;
}

//
// ICMPEchoRequestReceiver
//

bool ICMPEchoRequestReceiver::input(InetMessenger* m)
{
    esReport("ICMPEchoRequestReceiver::input\n");

    int len = m->getLength();
    ICMPEcho* icmphdr = static_cast<ICMPEcho*>(m->fix(len));

    int pos = 14 + 60;      // XXX Assume MAC, IPv4
    u8 chunk[pos + len];
    memmove(chunk + pos, icmphdr, len);
    icmphdr = reinterpret_cast<ICMPEcho*>(chunk + pos);
    icmphdr->type = ICMP_ECHO_REPLY;

    InetMessenger r(&InetReceiver::output, chunk, pos + len, pos);
    r.setRemote(m->getRemote());
    r.setLocal(m->getLocal());
    r.setType(IPPROTO_ICMP);

    Visitor v(&r);
    adapter->accept(&v);

    return true;
}

//
// ICMPEchoReplyReceiver
//

bool ICMPEchoReplyReceiver::input(InetMessenger* m)
{
    // Resume Inet4Address::isReachable()
    notify();

    return true;
}
