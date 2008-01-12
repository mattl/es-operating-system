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
#include <es/device/INetworkInterface.h>
#include <es/naming/IContext.h>
#include <es/net/ISocket.h>
#include <es/net/IInternetConfig.h>
#include <es/net/IResolver.h>
#include <es/net/arp.h>

using namespace es;

extern int esInit(IInterface** nameSpace);
extern void esRegisterInternetProtocol(IContext* context);

Handle<IResolver> resolver;

char buffer[64*1024];

// UDP Echo service - RFC 862
void echo(IInternetAddress* host)
{
    Handle<ISocket> socket = host->socket(AF_INET, ISocket::Datagram, 7);

    for (;;)
    {
        IInternetAddress* remoteAddress;
        int remotePort;

        int len = socket->recvFrom(buffer, sizeof buffer, 0, &remoteAddress, &remotePort);
        if (len < 0)
        {
            break;
        }

        if (remoteAddress)
        {
            InAddr remoteAddr;
            remoteAddress->getAddress(&remoteAddr, sizeof remoteAddr);

            esReport("'%*.*s', %08x:%d\n", len, len, buffer, ntohl(remoteAddr.addr), remotePort);

            socket->sendTo(buffer, len, 0, remoteAddress, remotePort);

            remoteAddress->release();
            remoteAddress = 0;
        }
    }

    socket->close();
}

int main()
{
    IInterface* root = NULL;
    esInit(&root);
    Handle<IContext> context(root);

    esRegisterInternetProtocol(context);

    // Create resolver object
    resolver = context->lookup("network/resolver");

    // Create internet config object
    Handle<IInternetConfig> config = context->lookup("network/config");

    // Setup DIX interface
    Handle<INetworkInterface> nic = context->lookup("device/ethernet");
    nic->start();
    int dixID = config->addInterface(nic);
    esReport("dixID: %d\n", dixID);

    ASSERT(config->getScopeID(nic) == dixID);

    // Register host address (192.168.2.40)
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 40) };
    Handle<IInternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof addr, dixID);
    config->addAddress(host, 16);
    esSleep(90000000);  // Wait for the host address to be settled.

    echo(host);

    esSleep(10000000);

    config->removeAddress(host);

    nic->stop();

    esReport("done.\n");
}
