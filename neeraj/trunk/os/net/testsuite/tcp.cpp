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

extern int esInit(Object** nameSpace);
extern es::Thread* esCreateThread(void* (*start)(void* param), void* param);

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

    es::Socket* socket;
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
    esReport("close() by serve()\n");
    socket->release();

    // Wait for 2 MSL time wait
    esSleep(2 * 1200000000LL + 100000000LL);

    return 0;   // lint
}

int main()
{
    Object* root = NULL;
    esInit(&root);
    Handle<es::Context> context(root);

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

    Socket raw(AF_INET, es::Socket::Raw);
    inProtocol = inFamily->getProtocol(&raw);
    visualize();

    // Setup loopback interface
    Handle<es::NetworkInterface> loopbackInterface = context->lookup("device/loopback");
    int scopeID = Socket::addInterface(loopbackInterface);

    // Register localhost address
    Handle<Inet4Address> localhost = new Inet4Address(InAddrLoopback, Inet4Address::statePreferred, scopeID);
    inFamily->addAddress(localhost);
    localhost->start();
    visualize();

    Socket socket(AF_INET, es::Socket::Stream);
    socket.bind(localhost, 54);
    visualize();

    es::Thread* thread = esCreateThread(serve, &socket);
    thread->start();

    esSleep(10000000);

    // Test bind and connect operations
    Socket client(AF_INET, es::Socket::Stream);
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
    esReport("close() by main()\n");
    visualize();

    void* val = thread->join();

    esReport("done.\n");
}
