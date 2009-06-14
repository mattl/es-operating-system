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

/*Discard TCP Server*/

#include <string.h>
#include "arp.h"
#include "conduit.h"
#include "dix.h"
#include "inet.h"
#include "inet4.h"
#include "inet4address.h"
#include "tcp.h"

extern int esInit(es::Interface** nameSpace);
extern void esRegisterInternetProtocol(es::Context* context);
static void* discard(void* param);
int n=0;

Handle<es::Resolver> resolver;

int main()
{
    es::Interface* root = NULL;
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

    //192.168.2.40 Register host address
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 40) };
    Handle<es::InternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof addr, dixID);
    config->addAddress(host, 16);
    esSleep(90000000);  // Wait for the host address to be settled.

    Handle<es::Socket> serverSocket = host->socket(AF_INET, es::Socket::Stream, 9);

    serverSocket->listen(5);

    es::Socket* discardServer;
    while (true)
    {
        while ((discardServer = serverSocket->accept()) == 0)
        {
        }

        esReport("accepted\n");

        discard(discardServer);

    }
    

    config->removeAddress(host);
    nic->stop();
    
    esReport("done.\n");
}
//discard service
static void* discard(void* param)
{
    n++;
    Socket* discardServer = static_cast<Socket*>(param);
    char input[9],prev[9];
    strcpy(prev,"dummy");
    int timeoutcount=0;
    while(timeoutcount<=10)
    {
       discardServer->read(input,9);
       if(strcmp(prev,input)==0)
       {
          timeoutcount++;
          esSleep(9000000);
       }
       else
       {
          //discard any data sent. Do not reply
          esReport("                    discarding \"%s\" by thread %d\n",input,n);
          strcpy(prev,input);
          timeoutcount=0;
       }
	  
       memset(input,0,9);
    }
    discardServer->close();
    discardServer->release();
}

