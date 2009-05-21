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



extern int esInit(Object** nameSpace);
extern void esRegisterInternetProtocol(es::Context* context);

void printAddress(es::InternetAddress* address)
{
    if (address)
    {
        InAddr addr;

        address->getAddress(&addr, sizeof(InAddr));
        char addrString[16];
        if (addr.ntoa(addrString, sizeof addrString))
        {
            esReport("%s\n", addrString);
        }

        char hostName[DNSHdr::NameMax];
        if (address->getHostName(hostName, sizeof hostName))
        {
            esReport("'%s'\n", hostName);
        }
    }
}

int main()
{
    Object* root = 0;
    esInit(&root);
    Handle<es::Context> context(root);

    esRegisterInternetProtocol(context);

    // Create resolver object
    Handle<es::Resolver> resolver = context->lookup("network/resolver");

    // Create internet config object
    Handle<es::InternetConfig> config = context->lookup("network/config");

    // Setup DIX interface
    Handle<es::NetworkInterface> ethernetInterface = context->lookup("device/ethernet");
    ethernetInterface->start();
    int dixID = config->addInterface(ethernetInterface);
    esReport("dixID: %d\n", dixID);

    InAddr addr, addrRouter, addrNameServer;
    // Register host address (192.168.2.40)
    addr.aton("192.168.2.40");
    Handle<es::InternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof(InAddr), dixID);
    config->addAddress(host, 16);
    esSleep(90000000);  // Wait for the host address to be settled.

    // Register a default router (192.168.2.1)
    addrRouter.aton("192.168.2.1");
    Handle<es::InternetAddress> router = resolver->getHostByAddress(&addrRouter.addr, sizeof(InAddr), dixID);
    config->addRouter(router);

    // Register a domain name server (192.168.2.1)
    addrNameServer.aton("192.168.2.1");
    Handle<es::InternetAddress> nameServer = resolver->getHostByAddress(&addrNameServer.addr, sizeof(InAddr), dixID);
    config->addNameServer(nameServer);

    config->addSearchDomain("nintendo.com");
    config->addSearchDomain("google.com");

    Handle<es::InternetAddress> address;

    // will resolve to www.nintendo.com
    address = resolver->getHostByName("www", AF_INET);
    printAddress(address);

    // will resolve to code.l.google.com
    address = resolver->getHostByName("code", AF_INET);
    printAddress(address);

    esSleep(10000000);
    ethernetInterface->stop();

    esReport("done.\n");
}
