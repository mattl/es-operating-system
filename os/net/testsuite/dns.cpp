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

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/interlocked.h>
#include <es/timeSpan.h>
#include <es/base/IStream.h>
#include <es/device/INetworkInterface.h>
#include <es/naming/IContext.h>
#include <es/net/arp.h>
#include <es/net/dns.h>
#include <es/net/ISocket.h>
#include <es/net/IInternetConfig.h>
#include <es/net/IResolver.h>

using namespace es;

extern int esInit(IInterface** nameSpace);
extern void esRegisterInternetProtocol(IContext* context);

int main()
{
    IInterface* root = 0;
    esInit(&root);
    Handle<IContext> context(root);

    esRegisterInternetProtocol(context);

    // Create resolver object
    Handle<IResolver> resolver = context->lookup("network/resolver");

    // Create internet config object
    Handle<IInternetConfig> config = context->lookup("network/config");

    // Setup DIX interface
    Handle<INetworkInterface> ethernetInterface = context->lookup("device/ethernet");
    ethernetInterface->start();
    int dixID = config->addInterface(ethernetInterface);
    esReport("dixID: %d\n", dixID);

    // Register host address (192.168.2.40)
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 40) };
    Handle<IInternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof(InAddr), dixID);
    config->addAddress(host, 16);
    esSleep(90000000);  // Wait for the host address to be settled.

    // Register a default router (192.168.2.1)
    InAddr addrRouter = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 1) };
    Handle<IInternetAddress> router = resolver->getHostByAddress(&addrRouter.addr, sizeof(InAddr), dixID);
    config->addRouter(router);

    // Register a domain name server (192.168.2.1)
    InAddr addrNameServer = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 1) };
    Handle<IInternetAddress> nameServer = resolver->getHostByAddress(&addrNameServer.addr, sizeof(InAddr), dixID);
    config->addNameServer(nameServer);

    Handle<IInternetAddress> address = resolver->getHostByName("www.nintendo.com", AF_INET);
    if (address)
    {
        InAddr addr;

        address->getAddress(&addr, sizeof(InAddr));
        u32 h = ntohl(addr.addr);
        esReport("%d.%d.%d.%d\n", (u8) (h >> 24), (u8) (h >> 16), (u8) (h >> 8), (u8) h);

        char hostName[DNSHdr::NameMax];
        if (address->getHostName(hostName, sizeof hostName))
        {
            esReport("'%s'\n", hostName);
        }
    }

    esSleep(10000000);
    ethernetInterface->stop();

    esReport("done.\n");
}
