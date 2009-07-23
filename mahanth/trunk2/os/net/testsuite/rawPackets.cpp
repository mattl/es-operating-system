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

/* Program to test passing and receiving of raw IP packets */

#include <es.h>
#include <es/handle.h>
#include <es/naming/IContext.h>
#include "tIn.h"
#include "inet.h"
#include "inet4.h"
#include "inet4address.h"
#include "inet.h"
#include <es/net/inet4.h>

extern int esInit(Object** nameSpace);
extern void esRegisterInternetProtocol(es::Context* context);
s16 checksum(const InetMessenger* m, int hlen);
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

    Handle<es::NetworkInterface> nic = context->lookup("device/ethernet");
    nic->start();
    TInterface *tin = new TInterface(nic);
    tin->start();

    InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 40) };
    es::InternetAddress *host = resolver->getHostByAddress(&addr.addr, sizeof addr, 2);

    InAddr addr1 = { htonl(192 << 24 | 168 << 16 | 2 << 8 |1 ) };
    es::InternetAddress *server = resolver->getHostByAddress(&addr1.addr, sizeof addr1, 2);
          
    int size = 14 + 60 + 60;
    InetMessenger *msg = new InetMessenger(&InetReceiver::output, size, size);
    msg->setLocal(dynamic_cast<Address*>(host));
    msg->setRemote(dynamic_cast<Address*>(server));
    msg->setType(IPPROTO_TCP);

    // create a test IP packet
    Handle<Inet4Address> addrIP;
    
    long len = msg->getLength();
    len += sizeof(IPHdr);
    msg->movePosition(-sizeof(IPHdr));
    msg->savePosition();
    IPHdr* iphdr = static_cast<IPHdr*>(msg->fix(sizeof(IPHdr)));
    iphdr->verlen = 0x45;
    iphdr->tos = 0;
    iphdr->len = htons(len);    // octets in header and data
    iphdr->id = htons(3); 
    iphdr->frag = 0;
    iphdr->ttl = 64;
    iphdr->proto = 6;    // for TCP
    iphdr->sum = 0;

    addrIP = msg->getLocal();
    addrIP->getAddress(&iphdr->src, sizeof(InAddr));
    msg->setScopeID(addrIP->getScopeID());

    addrIP = msg->getRemote();
    addrIP->getAddress(&iphdr->dst, sizeof(InAddr));
    u8 mac[6] = {0x0,0x16,0xd3,0x7,0xfa,0x5f};
    addrIP->setMacAddress(mac);
    iphdr->sum = checksum(msg, iphdr->getHdrSize());

    msg->setType(AF_INET);

    // send the IP packet
    Visitor v(msg);
    Protocol p = tin->inProtocol;
    p.accept(&v);

    // wait while the interface listens for IP packets
    while (1)
    {
        esSleep(80000000);
    }

    nic->stop();

    esReport("done.\n");
}

s16 checksum(const InetMessenger* m, int hlen)
{
    s32 sum = m->sumUp(hlen);
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return ~sum;
}

