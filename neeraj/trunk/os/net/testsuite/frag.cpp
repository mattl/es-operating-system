/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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
#include "tcp.h"
#include "udp.h"
#include "visualizer.h"

extern int esInit(Object** nameSpace);

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
    Object* root = NULL;
    esInit(&root);
    Handle<es::Context> context(root);

    Socket::initialize();

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
    Handle<Inet4Address> localhost = new Inet4Address(InAddrLoopback, Inet4Address::statePreferred, scopeID, 8);
    inFamily->addAddress(localhost);
    localhost->start();
    visualize();

    // Test bind and connect operations
    Socket socket(AF_INET, es::Socket::Datagram);
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
