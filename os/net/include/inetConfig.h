/*
 * Copyright 2008 Google Inc.
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

class InternetConfig : public IInternetConfig
{
    Ref                             ref;
    AddressSet<IInternetAddress>    nameServers;
    Collection<char*>               domains;

public:
    //
    // IInternetConfig
    //
    void addAddress(IInternetAddress* address, unsigned int prefix);
    IInternetAddress* getAddress(unsigned int scopeID);
    void removeAddress(IInternetAddress* address);

    void addRouter(IInternetAddress* router);
    IInternetAddress* getRouter();
    void removeRouter(IInternetAddress* router);

    int addInterface(INetworkInterface* networkInterface);
    IInterface* getInterface(int scopeID);
    int getScopeID(INetworkInterface* networkInterface);
    void removeInterface(INetworkInterface* networkInterface);

    void addNameServer(IInternetAddress* address);
    IInternetAddress* getNameServer();
    void removeNameServer(IInternetAddress* address);

    void addSearchDomain(const char* address);
    int getSearchDomain(char* address, int addressLength);
    int getSearchDomain(char* address, int addressLength, int pos);
    void removeSearchDomain(const char* address);

    //
    // IInterface
    //
    void* queryInterface(const Guid& riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // INETCONFIG_H_INCLUDED
