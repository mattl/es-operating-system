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

    // Test bind and connect operations
    Handle<IInternetAddress> loopback = resolver->getHostByAddress(&InAddrLoopback.addr, sizeof InAddrLoopback, 1);
    Handle<ISocket> socket = loopback->socket(AF_INET, ISocket::Datagram, 53);
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
