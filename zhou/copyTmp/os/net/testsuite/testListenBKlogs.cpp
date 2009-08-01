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

/*
  Test listen backlogs.
 */

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
    int dixID = config->addInterface(nic); // add Interface
    esReport("dixID: %d\n", dixID);

    ASSERT(config->getScopeID(nic) == dixID);

    // 192.168.2.20
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 20) };
    Handle<es::InternetAddress> host
        = resolver->getHostByAddress(&addr.addr, sizeof addr, dixID);
    config->addAddress(host, 16);
    esSleep(80000000);  // Wait for the host address to be settled.

    // Setup internet protocol family
    InFamily* inFamily = new InFamily;
    Socket* listenSocket = new Socket(AF_INET, es::Socket::Stream);

    /*
      Without calling accept() below to keep the accepted queue
      and then the queue can only be cleaned up by abort()
     */
    listenSocket->bind(host, 13);
    listenSocket->listen(5);
    
    // Get the StreamReceiver obj associated with listenSocket
    Adapter* adapter;
    adapter = listenSocket->getAdapter();
    Conduit* p =adapter->getA();
    StreamReceiver* s = dynamic_cast<StreamReceiver*>(p->getReceiver());

    // dump initial listen queue.
    s->dumpAccepted();

    esSleep(300000000); // period1
    // dump connection requests coming in period1
    s->dumpAccepted();
    
    esSleep(300000000); // period2
    /*
      Dump queue to see whether there are connections which came in period1
      aborted(eg. Reset by client)in period2,before this server call listenSocket->accept()
     */
    s->dumpAccepted();
   
    config->removeAddress(host);
    nic->stop();
    
    esReport("done.\n");
}
