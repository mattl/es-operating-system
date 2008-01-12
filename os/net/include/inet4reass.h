/*
 * Copyright (c) 2007
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

#ifndef INET4REASS_H_INCLUDED
#define INET4REASS_H_INCLUDED

#include "inet.h"

class ReassReceiver :
    public InetReceiver,
    public InetMessenger,
    public TimerTask
{
    static const u16 EOH = 1;   // end-of-hole mark (EOH % sizeof(Hole) != 0)

    struct Hole
    {
        u16 first;
        u16 last;
        u16 next;
        u16 _unused;
    };

    // The first fragment header needs to be saved for inclusion in a
    // possible ICMP Time Exceeded (Reassembly Timeout) message. [RFC 1122]
    u8              firstFragment[IPHdr::MaxHdrSize + 8];

    Protocol*       inProtocol;
    Protocol*       timeExceededProtocol;
    InetMessenger*  r;
    u16             list;

    Conduit*        adapter;

public:
    ReassReceiver(Protocol* inProtocol, Protocol* timeExceededProtocol, Conduit* adapter) :
        inProtocol(inProtocol),
        timeExceededProtocol(timeExceededProtocol),
        r(0),
        adapter(adapter)
    {
    }

    bool input(InetMessenger* m, Conduit* c);

    ReassReceiver* clone(Conduit* conduit, void* key)
    {
        return new ReassReceiver(inProtocol, timeExceededProtocol, conduit);
    }

    unsigned int release()
    {
        delete this;
        return 0;
    }

    void run();
};

class ReassFactoryReceiver : public InetReceiver
{
public:
    bool input(InetMessenger* m, Conduit* c);
};

#endif  // INET4REASS_H_INCLUDED
