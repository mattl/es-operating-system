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
  RFC-867
  TCP Based Daytime Protocol.
  Implemented by two threads of which the main thread is client.
  Using loopback interface.
*/

#include <string>
#include <sstream>
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

extern int esInit(Object** nameSpace);
extern es::Thread* esCreateThread(void* (*start)(void* param), void* param);


static void* serve(void* param)
{
    std::stringstream quote;
    std::string dayOfWeek[] =
        {
            "Sunday",
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday"
        };
    std::string months[] =
        {
            "January",
            "February",
            "March",
            "April",
            "May",
            "June",
            "July",
            "August",
            "September",
            "October",
            "November",
            "December"
        };
    
    Socket* listenSock = static_cast<Socket*>(param);
    listenSock->listen(5);

    es::Socket* connSocket;
    while (true)
    {
        while ((connSocket = listenSock->accept()) == 0)
        {
        }

        esReport("accepted\n");

        //Weekday, Month Day, Year Time-Zone
        DateTime d(DateTime::getNow());
        quote.str("");
        quote << dayOfWeek[d.getDayOfWeek()] << ", ";
        quote << months[d.getMonth()] << " ";
        quote << d.getDay() << ", ";
        quote << d.getYear() << " ";
        quote << d.getHour() << ":";
        quote << d.getMinute() << ":";
        quote << d.getSecond() << "-";
        quote << "UTC";

        std::string s = quote.str();
        connSocket->write(s.c_str(), s.length() + 1); // Sent current dateTime
        
        connSocket->close();
        connSocket->release();
    }
    
    return 0;
}

int main()
{
    Object* root = NULL;
    esInit(&root);
    Handle<es::Context> context(root);

    Socket::initialize();

    // Setup internet protocol family
    InFamily* inFamily = new InFamily;
   
    // Setup loopback interface
    Handle<es::NetworkInterface> loopbackInterface
        = context->lookup("device/loopback");

    int scopeID = Socket::addInterface(loopbackInterface);
    // Register localhost address
    Handle<Inet4Address> localhost
        = new Inet4Address(InAddrLoopback, Inet4Address::statePreferred, scopeID);
    inFamily->addAddress(localhost);
    localhost->start();
    

    Socket socket(AF_INET, es::Socket::Stream);
    socket.bind(localhost, 13);
    
    es::Thread* thread = esCreateThread(serve, &socket);
    thread->start(); // server thread start

    // client
    while(true)
    {
        esSleep(80000000);
        Socket client(AF_INET, es::Socket::Stream);
        client.connect(localhost, 13);
        
        char input[80];
        client.read(input, 80);

        esReport("%s\n",input);
        client.close();
    }

    void* val = thread->join();

    esReport("done.\n");
}
