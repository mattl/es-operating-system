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

#ifndef UDP_H_INCLUDED
#define UDP_H_INCLUDED

#include <es/endian.h>
#include <es/net/udp.h>
#include "inet.h"

// IPv4 UDP Receiver
class UDPReceiver : public InetReceiver
{
    s16 checksum(InetMessenger* m);

public:
    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
    bool error(InetMessenger* m, Conduit* c);
};

class UDPUnreachReceiver : public InetReceiver
{
    Protocol*   unreachProtocol;

public:
    UDPUnreachReceiver(Protocol* unreachProtocol) :
        unreachProtocol(unreachProtocol)
    {
    }

    bool input(InetMessenger* m, Conduit* c);
};

#endif  // UDP_H_INCLUDED
