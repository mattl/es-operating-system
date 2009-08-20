/*
 * Copyright 2009 Google Inc.
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
#include "arp.h"
#include "conduit.h"
#include "dix.h"
#include "inet.h"
#include "inet4.h"
#include "inet4address.h"
#include "tcp.h"

extern int esInit(Object** nameSpace);
extern void esRegisterInternetProtocol(es::Context* context);

Handle<es::Resolver> resolver;

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
    int dixID = config->addInterface(nic);// add Interface
    esReport("dixID: %d\n", dixID);

    ASSERT(config->getScopeID(nic) == dixID);

    // 192.168.2.40 Register host address
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 20) };
    Handle<es::InternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof addr, dixID);
    config->addAddress(host, 16);
    esSleep(90000000);  // Wait for the host address to be settled.

    Handle<es::Socket> serverSocket = host->socket(AF_INET, es::Socket::Stream, 9);

    serverSocket->listen(5);

    es::Socket* Server;
    while (true)
    {
        while ((Server = serverSocket->accept()) == 0)
        {
        }

        esReport("accepted\n");

        char data[9];
        int length;
        memset (data, 0, 9);


        while (true)
        {
            
	    // server checks whether the socket has entered urgent mode at every read
            if (Server->isUrgent())
            {
                esReport("Urgent Mode\n\n");
            }
            else
            {
                esReport("Not in urgent Mode\n\n");
            }
            
            // Tells whether the next read delivers an urgent byte            
            if (Server->sockAtMark())
            {
                esReport("An urgent byte will be read now\n");
            }

            length = Server->read(data, 3);
            esReport("Received data and length.......... ............ %s        %d\n", data, length);
            memset (data, 0, 3);
            if (length == 0)
            {
                break;
            }
        }
        Server->close();
        Server->release();
    }

    config->removeAddress(host);
    nic->stop();
    
    esReport("done.\n");
}

