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

#ifndef INETCONFIG_H_INCLUDED
#define INETCONFIG_H_INCLUDED

#include <string.h>
#include <es.h>
#include <es/ref.h>
#include <es/types.h>
#include <es/net/dns.h>
#include <es/net/IInternetConfig.h>
#include "inet4.h"
#include "socket.h"

class InternetConfig : public es::InternetConfig
{
    Ref                             ref;
    AddressSet<es::InternetAddress>    nameServers;
    Collection<char*>               domains;

public:
    //
    // IInternetConfig
    //
    void addAddress(es::InternetAddress* address, unsigned int prefix);
    es::InternetAddress* getAddress(unsigned int scopeID);
    void removeAddress(es::InternetAddress* address);

    void addRouter(es::InternetAddress* router);
    es::InternetAddress* getRouter();
    void removeRouter(es::InternetAddress* router);

    int addInterface(es::NetworkInterface* networkInterface);
    Object* getInterface(int scopeID);
    int getScopeID(es::NetworkInterface* networkInterface);
    void removeInterface(es::NetworkInterface* networkInterface);

    void addNameServer(es::InternetAddress* address);
    es::InternetAddress* getNameServer();
    void removeNameServer(es::InternetAddress* address);

    void addSearchDomain(const char* address);
    const char* getSearchDomain(void* address, int addressLength);
    const char* getSearchDomain(void* address, int addressLength, int pos);
    void removeSearchDomain(const char* address);

    //
    // IInterface
    //
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // INETCONFIG_H_INCLUDED
