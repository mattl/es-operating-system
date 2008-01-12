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

extern int esInit(IInterface** nameSpace);
extern IThread* esCreateThread(void* (*start)(void* param), void* param);
extern void esRegisterInternetProtocol(IContext* context);
extern void esRegisterDHCPClient(IContext* context);

int main()
{
    IInterface* root = NULL;
    esInit(&root);
    Handle<IContext> context(root);

    esRegisterInternetProtocol(context);

    // Lookup resolver object
    Handle<IResolver> resolver = context->lookup("network/resolver");

    // Lookup internet config object
    Handle<IInternetConfig> config = context->lookup("network/config");

    // Setup DIX interface
    Handle<INetworkInterface> ethernetInterface = context->lookup("device/ethernet");
    ethernetInterface->start();
    int dixID = config->addInterface(ethernetInterface);
    esReport("dixID: %d\n", dixID);

    esRegisterDHCPClient(context);

    Handle<IService> service = context->lookup("network/interface/2/dhcp");
    service->start();
    esSleep(120000000);

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
