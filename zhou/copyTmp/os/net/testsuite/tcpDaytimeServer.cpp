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
  RFC-867.
  TCP Based Daytime Protocol.
  Using ethernet interface.
  Test on ubuntu using vlan.
*/

#include <string>
#include <sstream>

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

//Daytime Protocol-RFC 867
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
            "Januarye",
            "February",
            "Marche",
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
        DateTime d(DateTime::getNow());//UTC time-daytime.h
        quote << dayOfWeek[d.getDayOfWeek()]<<", ";
        quote << months[d.getMonth()]<<" ";
        quote << d.getDay()<<", ";
        quote << d.getYear()<<" ";
        quote << d.getHour()<<":";
        quote << d.getMinute()<<":";
        quote << d.getSecond()<<"-";
        quote << "UTC";

        std::string s = quote.str();
        connSocket->write(s.c_str(),s.length()+1);//sent current date and time
        
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

    //192.168.0.8 (vlan 4 on my ubuntu)
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 0 << 8 | 8) };
    Handle<es::InternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof addr, dixID);
    config->addAddress(host, 16);
    esSleep(90000000);  // Wait for the host address to be settled.

    Handle<es::Socket> listenSocket = host->socket(AF_INET, es::Socket::Stream, 13);

    serve(listenSocket);
    

    config->removeAddress(host);
    nic->stop();
    
    esReport("done.\n");
}
