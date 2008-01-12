/*
 * Copyright (c) 2006
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#ifndef INET4_H_INCLUDED
#define INET4_H_INCLUDED

#include <es/endian.h>
#include <es/handle.h>
#include <es/tree.h>
#include <es/net/inet4.h>
#include "arp.h"
#include "datagram.h"
#include "icmp4.h"
#include "igmp.h"
#include "inet4address.h"
#include "scope.h"
#include "socket.h"
#include "stream.h"
#include "tcp.h"
#include "udp.h"

class ARPFamily;
class InFamily;

class InAccessor : public Accessor
{
public:
    /** @return the protocol field of IPHdr as the key.
     */
    void* getKey(Messenger* m)
    {
        ASSERT(m);

        IPHdr* iphdr = static_cast<IPHdr*>(m->fix(sizeof(IPHdr)));
        m->movePosition(iphdr->getHdrSize());
        return reinterpret_cast<void*>(iphdr->proto);
    }
};

class InReceiver : public InetReceiver
{
    static u16  identification;

    InFamily*   inFamily;

    s16 checksum(const InetMessenger* m, int hlen);

public:
    InReceiver(InFamily* inFamily) : inFamily(inFamily)
    {
    }

    bool input(InetMessenger* m);
    bool output(InetMessenger* m);
    bool error(InetMessenger* m);
};

class InFamily : public AddressFamily
{
    // Scope demultiplexer
    ScopeAccessor               scopeAccessor;
    ConduitFactory              scopeFactory;
    Mux                         scopeMux;

    // In protocol
    InReceiver                  inReceiver;
    Protocol                    inProtocol;
    InAccessor                  inAccessor;
    ConduitFactory              inFactory;
    Mux                         inMux;

    // ICMP
    ICMPReceiver                icmpReceiver;
    Protocol                    icmpProtocol;
    ICMPAccessor                icmpAccessor;
    ConduitFactory              icmpFactory;
    Mux                         icmpMux;

    // ICMP Echo Reply
    InetRemoteAddressAccessor   echoReplyAccessor;
    ConduitFactory              echoReplyFactory;
    Mux                         echoReplyMux;

    // ICMP Echo Request
    InetLocalAddressAccessor    echoRequestAccessor;
    Adapter                     echoRequestAdapter;     // prototype
    ICMPEchoRequestReceiver     echoRequestReceiver;    // prototype
    ConduitFactory              echoRequestFactory;
    Mux                         echoRequestMux;

    // ICMP Unreach
    // ICMP Redirect
    // ...

    // IGMP
    IGMPReceiver                igmpReceiver;
    Protocol                    igmpProtocol;
    IGMPAccessor                igmpAccessor;
    ConduitFactory              igmpFactory;
    Mux                         igmpMux;

    // UDP
    DatagramReceiver            datagramReceiver;
    Protocol                    datagramProtocol;
    InetRemoteAddressAccessor   udpRemoteAddressAccessor;
    ConduitFactory              udpRemoteAddressFactory;
    Mux                         udpRemoteAddressMux;
    InetRemotePortAccessor      udpRemotePortAccessor;
    ConduitFactory              udpRemotePortFactory;
    Mux                         udpRemotePortMux;
    InetLocalAddressAccessor    udpLocalAddressAccessor;
    ConduitFactory              udpLocalAddressFactory;
    Mux                         udpLocalAddressMux;
    InetLocalPortAccessor       udpLocalPortAccessor;
    ConduitFactory              udpLocalPortFactory;
    Mux                         udpLocalPortMux;
    UDPReceiver                 udpReceiver;
    Protocol                    udpProtocol;

    // TCP
    StreamReceiver              streamReceiver;
    Protocol                    streamProtocol;
    InetRemoteAddressAccessor   tcpRemoteAddressAccessor;
    ConduitFactory              tcpRemoteAddressFactory;
    Mux                         tcpRemoteAddressMux;
    InetRemotePortAccessor      tcpRemotePortAccessor;
    ConduitFactory              tcpRemotePortFactory;
    Mux                         tcpRemotePortMux;
    InetLocalAddressAccessor    tcpLocalAddressAccessor;
    ConduitFactory              tcpLocalAddressFactory;
    Mux                         tcpLocalAddressMux;
    InetLocalPortAccessor       tcpLocalPortAccessor;
    ConduitFactory              tcpLocalPortFactory;
    Mux                         tcpLocalPortMux;
    TCPReceiver                 tcpReceiver;
    Protocol                    tcpProtocol;

    // Inet4Address tree
    Tree<InAddr, Inet4Address*> addressTable[Socket::INTERFACE_MAX];

    // ARP family
    ARPFamily                   arpFamily;

    // Default Router List
    RouterList<Inet4Address>    routerList;

public:
    InFamily();

    int getAddressFamily()
    {
        return AF_INET;
    }

    Conduit* getProtocol(Socket* socket)
    {
        if (socket->getAddressFamily() == AF_INET)
        {
            switch (socket->getSocketType())
            {
            case ISocket::STREAM:
                return &tcpProtocol;
                break;
            case ISocket::DGRAM:
                return &udpProtocol;
                break;
            case ISocket::RAW:
                return &inProtocol;
                break;
            default:
                break;
            }
        }
        return 0;
    }

    void addInterface(Interface* interface)
    {
        scopeMux.addB(reinterpret_cast<void*>(interface->getScopeID()),
                      interface->addAddressFamily(this, &scopeMux));
    }

    Inet4Address* getAddress(InAddr addr, int scopeID = 0);
    void addAddress(Inet4Address* address);
    void removeAddress(Inet4Address* address);

    void addRouter(Inet4Address* addr)
    {
        Handle<Inet4Address> local;
        local = onLink(addr->getAddress(), addr->getScopeID());
        if (local)
        {
            routerList.addRouter(addr);
        }
    }

    void removeRouter(Inet4Address* addr)
    {
        routerList.removeRouter(addr);
    }

    Inet4Address* onLink(InAddr addr, int scopeID = 0);

    Inet4Address* getNextHop(Inet4Address* dst);
    Inet4Address* selectSourceAddress(Inet4Address* dst);

    bool isReachable(Inet4Address* address, long long timeout);

    // Inet4Address
    Inet4Address                addressAny;     // InAddrAny

    friend class Inet4Address;
    friend class Inet4Address::StateInit;
    friend class Inet4Address::StateTentative;
    friend class Inet4Address::StatePreferred;
};

#endif  // INET4_H_INCLUDED
