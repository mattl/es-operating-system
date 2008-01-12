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

#ifndef RESOLVER_H_INCLUDED
#define RESOLVER_H_INCLUDED

#include <string.h>
#include <es.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/types.h>
#include <es/net/IResolver.h>
#include "inet4.h"
#include "socket.h"

class Resolver : public IResolver
{
    Ref     ref;

public:
    //
    // IResolver
    //
    IInternetAddress* getHostByName(const char* hostName, int addressFamily);
    IInternetAddress* getHostByAddress(const void* address, unsigned int len, unsigned int scopeID);

    //
    // IInterface
    //
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef();
    unsigned int release();
};

#endif // RESOLVER_H_INCLUDED
