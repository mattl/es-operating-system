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



extern int esInit(Object** nameSpace);
extern void esRegisterInternetProtocol(es::Context* context);

Handle<es::Resolver> resolver;

char buffer[64*1024];

// UDP Echo service - RFC 862
void echo(es::InternetAddress* host)
{
    Handle<es::Socket> socket = host->socket(AF_INET, es::Socket::Datagram, 7);

    for (;;)
    {
        es::InternetAddress* remoteAddress;
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
    Object* root = NULL;
    esInit(&root);
    Handle<es::Context> context(root);

    esRegisterInternetProtocol(context);

    // Create resolver object
    resolver = context->lookup("network/resolver");

    // Create internet config object
    Handle<es::InternetConfig> config = context->lookup("network/config");

    // Setup DIX interface
    Handle<es::NetworkInterface> nic = context->lookup("device/ethernet");
    nic->start();
    int dixID = config->addInterface(nic);
    esReport("dixID: %d\n", dixID);

    ASSERT(config->getScopeID(nic) == dixID);

    // Register host address (192.168.2.40)
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 40) };
    Handle<es::InternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof addr, dixID);
    config->addAddress(host, 16);
    esSleep(90000000);  // Wait for the host address to be settled.

    echo(host);

    esSleep(10000000);

    config->removeAddress(host);

    nic->stop();

    esReport("done.\n");
}
