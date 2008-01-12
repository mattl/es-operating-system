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

#ifndef INETCONFIG_H_INCLUDED
#define INETCONFIG_H_INCLUDED

#include <string.h>
#include <es.h>
#include <es/ref.h>
#include <es/types.h>
#include <es/net/IInternetConfig.h>
#include "inet4.h"
#include "socket.h"

class InternetConfig : public IInternetConfig
{
    Ref                             ref;
    AddressSet<IInternetAddress>    nameServers;

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

    //
    // IInterface
    //
    void* queryInterface(const Guid& riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // INETCONFIG_H_INCLUDED
