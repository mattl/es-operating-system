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



#include <string>
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

// TCP Chargen Service - 864
void chargen(es::InternetAddress* host)
{
    std::string line;
    Handle<es::Socket> serverSocket = host->socket(AF_INET, es::Socket::Stream, 19);
    serverSocket->listen(5);

    es::Socket* clientSocket;
    while(true)
    {
        while((clientSocket = serverSocket->accept())==0)
        {
        }
        esReport("accepted\n");

        int i = 32;
        int j = 0;
        while(true)
        {
            line = "";
            for(j=0; j<=71; j++)
            {
                line.append(1,(char)(i+j));
            }
            line.append(1,(char)13);
            line.append(1,(char)10);
            i++;
            if(i == 56)
            {
                i = 32;
            }
            if(clientSocket->isConnected())
            {
                clientSocket->write(line.c_str(), line.length()+1);
            }
            else
            {
                break;
            }
        }        
        clientSocket->close();
        clientSocket->release();
    }

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

    // Wait for the host address to be settled
    esSleep(90000000);

    chargen(host);

    esSleep(10000000);

    config->removeAddress(host);

    nic->stop();

    esReport("done.\n");

}
