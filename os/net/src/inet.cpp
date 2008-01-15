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
