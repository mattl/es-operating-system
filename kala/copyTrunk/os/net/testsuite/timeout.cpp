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

#include <string.h>
#include <es.h>
#include <es/endian.h>
#include <es/handle.h>
#include <es/list.h>
#include <es/base/IStream.h>
#include <es/device/INetworkInterface.h>
#include <es/naming/IContext.h>
#include <es/net/ISocket.h>
#include <es/net/IInternetConfig.h>
#include <es/net/IResolver.h>
#include <es/net/arp.h>



extern int esInit(es::Interface** nameSpace);
extern void esRegisterInternetProtocol(es::Context* context);

int main()
{
    es::Interface* root = NULL;
    esInit(&root);
    Handle<es::Context> context(root);

    esRegisterInternetProtocol(context);

    // Create resolver object
    Handle<es::Resolver> resolver = context->lookup("network/resolver");

    // Create internet config object
    Handle<es::InternetConfig> config = context->lookup("network/config");

    // Test bind and connect operations
    Handle<es::InternetAddress> loopback = resolver->getHostByAddress(&InAddrLoopback.addr, sizeof InAddrLoopback, 1);
    Handle<es::Socket> socket = loopback->socket(AF_INET, es::Socket::Datagram, 53);
    socket->connect(loopback, 53);

    // Test read timeout
    esReport("setTimeout\n");
    socket->setTimeout(50000000);
    char input[4];
    esReport("read: ");
    int err = socket->read(input, 4);
    esReport("%d\n", err);

    // Test close operation
    socket->close();

    esReport("done.\n");
}
