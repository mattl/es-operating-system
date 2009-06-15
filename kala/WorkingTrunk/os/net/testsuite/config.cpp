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

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))



extern int esInit(Object** nameSpace);
extern void esRegisterInternetProtocol(es::Context* context);

int main()
{
    Object* root = NULL;
    esInit(&root);
    Handle<es::Context> context(root);

    esRegisterInternetProtocol(context);

    // Create resolver object
    Handle<es::Resolver> resolver = context->lookup("network/resolver");

    // Create internet config object
    Handle<es::InternetConfig> config = context->lookup("network/config");

    // Test local ping
    Handle<es::InternetAddress> loopback = resolver->getHostByAddress(&InAddrLoopback.addr, sizeof InAddrLoopback, 1);
    loopback->isReachable(10000000);

    // Test bind and connect operations
    Handle<es::Socket> socket = loopback->socket(AF_INET, es::Socket::Datagram, 53);
    socket->connect(loopback, 53);

    // Test read and write operations
    char output[4] = "xyz";
    socket->write(output, 4);

    char input[4];
    es::InternetAddress* remoteAddress;
    int remotePort;
    socket->recvFrom(input, 4, 0, &remoteAddress, &remotePort);
    esReport("'%s', %d\n", input, remotePort);
    ASSERT(remoteAddress->isLoopback());
    if (remoteAddress)
    {
        remoteAddress->release();
        remoteAddress = 0;
    }

    // Test close operation
    TEST(!socket->isClosed());
    socket->close();
    // TEST(socket->isClosed());    // XXX This test doesn't work now...

    // Setup DIX interface
    Handle<es::NetworkInterface> nic = context->lookup("device/ethernet");
    nic->start();
    int dixID = config->addInterface(nic);
    esReport("dixID: %d\n", dixID);

    ASSERT(config->getScopeID(nic) == dixID);

    // Register host address (192.168.2.40)
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 40) };
    Handle<es::InternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof addr, dixID);
    config->addAddress(host, 16);
    esSleep(90000000);  // Wait for the host address to be settled.

    // Register a default router (192.168.2.1)
    InAddr addrRouter = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 1) };
    Handle<es::InternetAddress> router = resolver->getHostByAddress(&addrRouter.addr, sizeof addr, dixID);
    config->addRouter(router);

    // Test remote ping
    for (int i = 1; i <= 3; ++i)
    {
        bool reachable = router->isReachable(10000000);
        esReport("ping #%d: %d\n", i, reachable);
    }

    esSleep(10000000);

    config->removeRouter(router);
    config->removeAddress(host);

    nic->stop();

    esReport("done.\n");
}
