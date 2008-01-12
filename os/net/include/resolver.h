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

#ifndef RESOLVER_H_INCLUDED
#define RESOLVER_H_INCLUDED

#include <string.h>
#include <es.h>
#include <es/interlocked.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/synchronized.h>
#include <es/types.h>
#include <es/base/IMonitor.h>
#include <es/net/dns.h>
#include <es/net/IResolver.h>
#include "inet4.h"
#include "socket.h"

class Resolver : public IResolver
{
    class Control
    {
        // the minimum retransmission interval should be 2-5 seconds [RFC 1035]
        static const int MinWait = 2;   // [sec]
        static const int MaxQuery = 3;

        Handle<IInternetAddress>    server;
        Handle<ISocket>             socket;

        char        suffix[DNSHdr::NameMax];
        Interlocked id;
        u8          query[1472];
        u8          response[1472];

        int a(u16 id, const char* hostName);
        int ptr(u16 id, InAddr addr);

        static u8* skipName(const u8* ptr, const u8* end);
        static bool copyName(const u8* dns, const u8* ptr, const u8* end, char* name);

    public:
        Control(IInternetAddress* server);
        ~Control();
        IInternetAddress* getHostByName(const char* hostName, int addressFamily);
        bool getHostName(IInternetAddress* address, char* hostName, unsigned int len);
    };

    Ref         ref;
    IMonitor*   monitor;
    Control*    control;

    bool setup();

public:
    Resolver();
    ~Resolver();

    //
    // IResolver
    //
    IInternetAddress* getHostByName(const char* hostName, int addressFamily);
    IInternetAddress* getHostByAddress(const void* address, unsigned int len, unsigned int scopeID);
    bool getHostName(IInternetAddress* address, char* hostName, unsigned int len);

    //
    // IInterface
    //
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef();
    unsigned int release();
};

#endif // RESOLVER_H_INCLUDED
