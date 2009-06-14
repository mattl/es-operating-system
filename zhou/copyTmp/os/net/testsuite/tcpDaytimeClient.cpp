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
#include "loopback.h"
#include "tcp.h"

//Daytime Protocol-RFC 867,client test program.

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

    //192.168.1.187--eth0 address on my ubuntu
    InAddr addr = { htonl(192 << 24 | 168 << 16 | 1 << 8 | 187) };
    Handle<es::InternetAddress> host = resolver->getHostByAddress(&addr.addr, sizeof addr, dixID);
    config->addAddress(host, 16);
    esSleep(80000000);  // Wait for the host address to be settled.


    //192.168.0.8,vlan 4 on my ubuntu
    InAddr addr2 = { htonl(192 << 24 | 168 << 16 | 0 << 8 | 8) };
    Handle<es::InternetAddress> server = resolver->getHostByAddress(&addr2.addr, sizeof addr2, dixID);
    
    
    Socket client(AF_INET, es::Socket::Stream);

    client.connect(server, 13);

    //Daytime from server
    char input[80];
    client.read(input, 80);
    esReport("\n%s\n", input);

    client.close();
    esReport("close() by main()\n");

    config->removeAddress(host);
    nic->stop();

    esReport("done.\n");
}
