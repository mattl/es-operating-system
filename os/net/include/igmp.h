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

#ifndef IGMP_H_INCLUDED
#define IGMP_H_INCLUDED

#include <es/endian.h>
#include <es/net/igmp.h>
#include "inet.h"

class IGMPReceiver :
    public InetReceiver
{
    InFamily*   inFamily;

public:
    IGMPReceiver(InFamily* inFamily) :
        inFamily(inFamily)
    {
    }

    s16 checksum(InetMessenger* m);
    bool input(InetMessenger* m);
    bool output(InetMessenger* m);
};

#endif  // IGMP_H_INCLUDED
