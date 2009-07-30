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

/* Test Program to Send an Urgent Byte */

#include <es.h>
#include <es/handle.h>
#include <es/naming/IContext.h>
#include "inet.h"
#include "inet4.h"
#include "inet4address.h"

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
    int dixID = config->addInterface(nic);
    esReport("dixID: %d\n", dixID);

    ASSERT(config->getScopeID(nic) == dixID);

    // 192.168.2.40 Register host address
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 40) };
    Handle<es::InternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof addr, dixID);
    config->addAddress(host, 16);
    esSleep(80000000);  // Wait for the host address to be settled.

    // 192.168.2.20
    InAddr addr1 = { htonl(192 << 24 | 168 << 16 | 2 << 8 |20 ) };
    Handle<es::InternetAddress> server = resolver->getHostByAddress(&addr1.addr, sizeof addr1, dixID);    
    
    Socket urgSend(AF_INET, es::Socket::Stream);

    // connect to a tcp server 
    urgSend.connect(server, 9);
    
    // send an urgent byte
    esReport("%d",es::Socket::MSG_OOB);
    urgSend.sendTo("c",1,es::Socket::MSG_OOB,server,9);
    
    // close the connection
    urgSend.close();

    config->removeAddress(host);
    nic->stop();

    esReport("done.\n");
}

