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

#ifndef IGMP_H_INCLUDED
#define IGMP_H_INCLUDED

#include <es/endian.h>
#include <es/net/igmp.h>
#include "inet.h"

class IGMPAccessor : public Accessor
{
public:
    /** @return the addr field of IGMPHdr as the key.
     */
    void* getKey(Messenger* m)
    {
        ASSERT(m);

        IGMPHdr* igmphdr = static_cast<IGMPHdr*>(m->fix(sizeof(IGMPHdr)));
        return reinterpret_cast<void*>(igmphdr->addr.addr);
    }
};

class IGMPReceiver :
    public InetReceiver
{
public:
    /** Validates the received IGMP packet.
     */
    bool input(InetMessenger* m)
    {
        IGMPHdr* igmphdr = static_cast<IGMPHdr*>(m->fix(sizeof(IGMPHdr)));
        return true;
    }

    bool output(InetMessenger* m)
    {
        IGMPHdr* igmphdr = static_cast<IGMPHdr*>(m->fix(sizeof(IGMPHdr)));
        return true;
    }
};

#endif  // IGMP_H_INCLUDED
