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

    // Setup loopback interface
    Handle<IStream> loopbackStream = context->lookup("device/loopback");
    int scopeID = Socket::addInterface(loopbackStream, ARPHdr::HRD_LOOPBACK);

    // Register localhost address
    Handle<Inet4Address> localhost = new Inet4Address(InAddrLoopback, Inet4Address::statePreferred, scopeID, 8);
    inFamily->addAddress(localhost);
    localhost->start();

    // Register resolver interface
    Resolver* resolver = new Resolver;
    context->bind("network/resolver", static_cast<IResolver*>(resolver));

    // Register config interface
    InternetConfig* config = new InternetConfig;
    context->bind("network/config", static_cast<InternetConfig*>(config));
}
