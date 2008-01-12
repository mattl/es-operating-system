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

#include <es.h>
#include <es/handle.h>
#include <es/naming/IContext.h>
#include "arp.h"
#include "conduit.h"
#include "datagram.h"
#include "dix.h"
#include "icmp4.h"
#include "igmp.h"
#include "inet.h"
#include "inet4.h"
#include "inet4address.h"
#include "inet6.h"
#include "inet6address.h"
#include "inetConfig.h"
#include "loopback.h"
#include "resolver.h"
#include "scope.h"
#include "tcp.h"
#include "udp.h"
#include "visualizer.h"

extern int esInit(IInterface** nameSpace);

Conduit* inProtocol;

void visualize()
{
    Visualizer v;
    inProtocol->accept(&v);
}

const int Size = 1500;
char output[Size];
char input[Size];

int main()
{
    IInterface* root = NULL;
    esInit(&root);
    Handle<IContext> context(root);

    Socket::initialize();

    // Setup internet protocol family
    InFamily* inFamily = new InFamily;
    esReport("AF: %d\n", inFamily->getAddressFamily());

    Socket raw(AF_INET, ISocket::Raw);
    inProtocol = inFamily->getProtocol(&raw);
    visualize();

    // Setup loopback interface
    Handle<IStream> loopbackStream = context->lookup("device/loopback");
    int scopeID = Socket::addInterface(loopbackStream, ARPHdr::HRD_LOOPBACK);

    // Register localhost address
    Handle<Inet4Address> localhost = new Inet4Address(InAddrLoopback, Inet4Address::statePreferred, scopeID, 8);
    inFamily->addAddress(localhost);
    localhost->start();
    visualize();

    // Test bind and connect operations
    Socket socket(AF_INET, ISocket::Datagram);
    socket.bind(localhost, 53);
    visualize();
    socket.connect(localhost, 53);
    visualize();

    int len;

    // Test read and write operations #1

    memset(output, '0', Size);
    memset(input, '1', Size);

    len = socket.write(output, sizeof output);
    esReport("socket.write: %d\n", len);

    len = socket.read(input, sizeof input);
    esReport("socket.read: %d\n", len);

    ASSERT(memcmp(output, input, Size) == 0);
    visualize();

    // Test read and write operations #2

    memset(output, '2', Size);
    memset(input, '3', Size);

    len = socket.write(output, sizeof output);
    esReport("socket.write: %d\n", len);

    len = socket.read(input, sizeof input);
    esReport("socket.read: %d\n", len);

    ASSERT(memcmp(output, input, Size) == 0);
    visualize();

    // Test close operation
    socket.close();
    visualize();

    esSleep(10000000);
    visualize();

    esReport("done.\n");
}
