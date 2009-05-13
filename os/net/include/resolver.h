/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

class Resolver : public es::Resolver
{
    class Control
    {
        // the minimum retransmission interval should be 2-5 seconds [RFC 1035]
        static const int MinWait = 2;   // [sec]
        static const int MaxQuery = 3;
        static const int MaxSearch = 3; // max search domains used

        Handle<es::InternetAddress>    server;
        Handle<es::Socket>             socket;

        char        suffix[DNSHdr::NameMax];
        Interlocked id;
        u8          query[1472];
        u8          response[1472];

        int a(u16 id, const char* hostName);
        int ptr(u16 id, InAddr addr);
        es::InternetAddress* resolve(const char* hostName);

        static u8* skipName(const u8* ptr, const u8* end);
        static bool copyName(const u8* dns, const u8* ptr, const u8* end, char* name);

    public:
        Control(es::InternetAddress* server);
        ~Control();
        es::InternetAddress* getHostByName(const char* hostName, int addressFamily);
        bool getHostName(es::InternetAddress* address, char* hostName, unsigned int len);
    };

    Ref         ref;
    es::Monitor*   monitor;
    Control*    control;

    bool setup();

public:
    Resolver();
    ~Resolver();

    //
    // IResolver
    //
    es::InternetAddress* getHostByName(const char* hostName, int addressFamily);
    es::InternetAddress* getHostByAddress(const void* address, int len, unsigned int scopeID);
    const char* getHostName(void* hostName, int len, es::InternetAddress* address);

    //
    // IInterface
    //
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // RESOLVER_H_INCLUDED
