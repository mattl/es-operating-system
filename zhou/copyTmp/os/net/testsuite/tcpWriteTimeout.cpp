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

#include <es.h>
#include <es/handle.h>
#include <es/naming/IContext.h>
#include "inet.h"
#include "inet4.h"
#include "inet4address.h"
#include "loopback.h"
#include "tcp.h"
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
    Socket* listening = static_cast<Socket*>(param);
    listening->listen(5);

    es::Socket* socket;
    while ((socket = listening->accept()) == 0)
    {
    }

    esReport("accepted\n");

    // Let the client fill up its buffer
    // Do not read anything
    while (true)
    {
	esSleep(30000000);
    }
    
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

    // Setup internet protocol family
    InFamily* inFamily = new InFamily;
    esReport("AF: %d\n", inFamily->getAddressFamily());

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

    Socket client(AF_INET, es::Socket::Stream);
    client.bind(localhost, 53);
    visualize();

    client.connect(localhost, 54);
    visualize();

    int len;

    char output[31] = "abcabcabcabcabcabcabcabcabcabc";

    client.setTimeout(30000000);
    while (true)    
    {
        len = client.write(output, 31);
        esReport("		length written is %d\n",len);
        if (len == -ETIMEDOUT)
        {
          esReport("		write timedout now exit\n");
          break;
        }
    }

    // Wait for close
    client.close();
    esReport("close() by main()\n");
    visualize();

    void* val = thread->join();

    esReport("done.\n");
}
