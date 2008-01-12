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

#include <algorithm>
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
#include "loopback.h"
#include "tcp.h"
#include "udp.h"
#include "visualizer.h"

extern int esInit(IInterface** nameSpace);
extern IThread* esCreateThread(void* (*start)(void* param), void* param);

Conduit* inProtocol;

void visualize()
{
#if 0
    Visualizer v;
    inProtocol->accept(&v);
#endif
}

static void* serve(void* param)
{
    // Test listen and accept operations
    Socket* listening = static_cast<Socket*>(param);
    listening->listen(5);

    ISocket* socket;
    while ((socket = listening->accept()) == 0)
    {
    }

    esReport("accepted\n");

    int len;

    char input[4];
    len = socket->read(input, 4);
    esReport("read: %s (%d)\n", input, len);
    ASSERT(len == 4);
    ASSERT(memcmp(input, "abc", 4) == 0);

    socket->close();
    socket->release();

    // Wait for 2 MSL time wait
    esSleep(2 * 1200000000LL + 100000000LL);

    return 0;   // lint
}

int main()
{
    IInterface* root = NULL;
    esInit(&root);
    Handle<IContext> context(root);

    Socket::initialize();

    esReport("TCPHdr: %d byte\n", sizeof(TCPHdr));
    ASSERT(sizeof(TCPHdr) == TCPHdr::MIN_HLEN);

    // Test TCPSeq
    TCPSeq a = 2147483647;
    TCPSeq b = a + 10;
    esReport("a: = %d, b = %d\n", (s32) a, (s32) b);
    ASSERT(a < b);
    ASSERT(b > a);
    ASSERT(TCPSeq(2147483647) < b);
    ASSERT(std::max(a, b) == b);
    ASSERT(std::min(a, b) == a);

    // Setup internet protocol family
    InFamily* inFamily = new InFamily;
    esReport("AF: %d\n", inFamily->getAddressFamily());

    Socket raw(AF_INET, ISocket::Raw);
    inProtocol = inFamily->getProtocol(&raw);
    visualize();

    // Setup loopback interface
    Handle<INetworkInterface> loopbackInterface = context->lookup("device/loopback");
    int scopeID = Socket::addInterface(loopbackInterface);

    // Register localhost address
    Handle<Inet4Address> localhost = new Inet4Address(InAddrLoopback, Inet4Address::statePreferred, scopeID);
    inFamily->addAddress(localhost);
    localhost->start();
    visualize();

    Socket socket(AF_INET, ISocket::Stream);
    socket.bind(localhost, 54);
    visualize();

    IThread* thread = esCreateThread(serve, &socket);
    thread->start();

    esSleep(10000000);

    // Test bind and connect operations
    Socket client(AF_INET, ISocket::Stream);
    client.bind(localhost, 53);
    visualize();

    client.connect(localhost, 54);
    visualize();

    int len;

    char output[4] = "abc";
    len = client.write(output, 4);
    ASSERT(len == 4);

    // Wait for close
    char input[4];
    client.read(input, 4);

    client.close();
    visualize();

    void* val;
    thread->join(&val);

    esReport("done.\n");
}
