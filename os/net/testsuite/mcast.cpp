/*
 * Copyright (c) 2007
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
#include <es/endian.h>
#include <es/handle.h>
#include <es/list.h>
#include <es/base/IStream.h>
#include <es/device/IEthernet.h>
#include <es/naming/IContext.h>
#include <es/net/ISocket.h>
#include <es/net/IInternetConfig.h>
#include <es/net/IResolver.h>
#include <es/net/arp.h>

extern int esInit(IInterface** nameSpace);
extern void esRegisterInternetProtocol(IContext* context);

int main()
{
    IInterface* root = NULL;
    esInit(&root);
    Handle<IContext> context(root);

    esRegisterInternetProtocol(context);

    // Create resolver object
    Handle<IResolver> resolver = context->lookup("network/resolver");

    // Create internet config object
    Handle<IInternetConfig> config = context->lookup("network/config");

    // Setup DIX interface
    Handle<IStream> ethernetStream = context->lookup("device/ethernet");
    Handle<IEthernet> nic(ethernetStream);
    nic->start();
    int dixID = config->addInterface(ethernetStream, ARPHdr::HRD_ETHERNET);

    // Register host address (192.168.2.40)
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 40) };
    Handle<IInternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof addr, dixID);
    config->addAddress(host, 16);
    esSleep(80000000);  // Wait for the host address to be settled.

    // Register a default router (192.168.2.1)
    InAddr addrRouter = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 1) };
    Handle<IInternetAddress> router = resolver->getHostByAddress(&addrRouter.addr, sizeof addr, dixID);
    config->addRouter(router);

    // Test join and leave (239.255.255.250 / SSDP)
    Handle<IMulticastSocket> socket = host->socket(AF_INET, ISocket::Datagram, 1900);
    InAddr ssdp = { htonl(239 << 24 | 255 << 16 | 255 << 8 | 250) };
    Handle<IInternetAddress> group = resolver->getHostByAddress(&ssdp.addr, sizeof ssdp, dixID);
    socket->joinGroup(group);

    esSleep(120000000);

    socket->leaveGroup(group);

    // Test close operation
    socket->close();
    socket = 0;

    esSleep(20000000);
    nic->stop();

    esReport("done.\n");
}
