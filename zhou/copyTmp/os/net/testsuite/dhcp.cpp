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
#include <es/dateTime.h>
#include <es/endian.h>
#include <es/handle.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/base/IService.h>
#include <es/base/IStream.h>
#include <es/base/IThread.h>
#include <es/device/INetworkInterface.h>
#include <es/naming/IContext.h>
#include <es/net/ISocket.h>
#include <es/net/IInternetConfig.h>
#include <es/net/IResolver.h>
#include <es/net/arp.h>
#include <es/net/dhcp.h>
#include <es/net/dns.h>
#include <es/net/udp.h>



extern int esInit(Object** nameSpace);
extern es::Thread* esCreateThread(void* (*start)(void* param), void* param);
extern void esRegisterInternetProtocol(es::Context* context);
extern void esRegisterDHCPClient(es::Context* context);

int main()
{
    Object* root = NULL;
    esInit(&root);
    Handle<es::Context> context(root);

    esRegisterInternetProtocol(context);

    // Lookup resolver object
    Handle<es::Resolver> resolver = context->lookup("network/resolver");

    // Lookup internet config object
    Handle<es::InternetConfig> config = context->lookup("network/config");

    // Setup DIX interface
    Handle<es::NetworkInterface> ethernetInterface = context->lookup("device/ethernet");
    ethernetInterface->start();
    int dixID = config->addInterface(ethernetInterface);
    esReport("dixID: %d\n", dixID);

    esRegisterDHCPClient(context);

    Handle<es::Service> service = context->lookup("network/interface/2/dhcp");
    service->start();
    esSleep(120000000);

    Handle<es::InternetAddress> host = config->getAddress(dixID);
    if (host)
    {
        InAddr addr;

        host->getAddress(&addr, sizeof(InAddr));
        u32 h = ntohl(addr.addr);
        esReport("host: %d.%d.%d.%d\n", (u8) (h >> 24), (u8) (h >> 16), (u8) (h >> 8), (u8) h);
    }

    Handle<es::InternetAddress> address = resolver->getHostByName("www.nintendo.com", AF_INET);
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

        // Test remote ping
        esReport("ping #1\n");
        address->isReachable(10000000);
        esReport("ping #2\n");
        address->isReachable(10000000);
        esReport("ping #3\n");
        address->isReachable(10000000);
    }

    service->stop();

    ethernetInterface->stop();

    esReport("done.\n");
}
