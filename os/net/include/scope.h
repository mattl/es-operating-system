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

#ifndef SCOPE_H_INCLUDED
#define SCOPE_H_INCLUDED

#include <es.h>
#include <es/handle.h>
#include <es/endian.h>
#include <es/net/dix.h>
#include "inet.h"

class ScopeAccessor : public Accessor
{
public:
    /** @return scopeID of the source address.
     */
    void* getKey(Messenger* m)
    {
        InetMessenger* im = dynamic_cast<InetMessenger*>(m);
        ASSERT(im);
        Handle<Address> addr = im->getLocal();
        int id = addr->getScopeID();
        return reinterpret_cast<void*>(id);
    }
};

#endif  // SCOPE_H_INCLUDED
