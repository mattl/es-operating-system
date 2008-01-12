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

#include <es/handle.h>
#include <es/naming/IContext.h>
#include "socket.h"
#include "inet4.h"
#include "inet6.h"
#include "resolver.h"
#include "inetConfig.h"

void esRegisterInternetProtocol(IContext* context)
{
    Socket::initialize();

    // Setup internet protocol family
    InFamily* inFamily = new InFamily;

    // Create "network/interface" context
    Socket::interface = context->createSubcontext("network/interface");

    // Register resolver interface
    Socket::resolver = new Resolver;
    Handle<IBinding>(context->bind("network/resolver", Socket::resolver));

    // Register config interface
    Socket::config = new InternetConfig;
    Handle<IBinding>(context->bind("network/config", Socket::config));

    // Setup loopback interface
    Handle<INetworkInterface> loopbackInterface = context->lookup("device/loopback");
    int scopeID = Socket::config->addInterface(loopbackInterface);

    // Register localhost address
    Handle<Inet4Address> localhost = new Inet4Address(InAddrLoopback, Inet4Address::statePreferred, scopeID, 8);
    inFamily->addAddress(localhost);
    localhost->start();
}
