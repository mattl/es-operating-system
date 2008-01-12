/*
 * Copyright (c) 2006, 2007
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
#include "inet4reass.h"
#include "scope.h"
#include "socket.h"
#include "stream.h"
#include "tcp.h"
#include "udp.h"

class ARPFamily;
class InFamily;

class InReceiver : public InetReceiver
{
    static u16  identification;

    InFamily*   inFamily;

    s16 checksum(const InetMessenger* m, int hlen);
    void fragment(const InetMessenger* m, int mtu, Address* nextHop);

public:
    InReceiver(InFamily* inFamily) : inFamily(inFamily)
    {
    }

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
    bool error(InetMessenger* m, Conduit* c);
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
    TypeAccessor                inAccessor;
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
    ICMPUnreachReceiver         unreachReceiver;
    Protocol                    unreachProtocol;

    // ICMP Time Exceeded
    ICMPTimeExceededReceiver    timeExceededReceiver;
    Protocol                    timeExceededProtocol;

    // ICMP Redirect
    // ...

    // IGMP
    IGMPReceiver                igmpReceiver;
    Protocol                    igmpProtocol;
    InetLocalAddressAccessor    igmpAccessor;
    Adapter                     igmpAdapter;            // prototype
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
    int                         udpLast;
    UDPUnreachReceiver          udpUnreachReceiver;

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
    int                         tcpLast;

    // Fragment Reassemble
    ReassReceiver               reassReceiver;
    Adapter                     reassAdapter;
    InetRemotePortAccessor      reassIdAccessor;
    ConduitFactory              reassIdFactory;
    Mux                         reassIdMux;
    InetRemoteAddressAccessor   reassRemoteAddressAccessor;
    ConduitFactory              reassRemoteAddressFactory;
    Mux                         reassRemoteAddressMux;
    InetLocalAddressAccessor    reassLocalAddressAccessor;
    ConduitFactory              reassLocalAddressFactory;
    Mux                         reassLocalAddressMux;
    Protocol                    reassProtocol;
    ReassFactoryReceiver        reassFactoryReceiver;

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
            case ISocket::Stream:
                return &tcpProtocol;
                break;
            case ISocket::Datagram:
                return &udpProtocol;
                break;
            case ISocket::Raw:
                return &inProtocol;
                break;
            default:
                break;
            }
        }
        return 0;
    }

    int selectEphemeralPort(Socket* socket)
    {
        int* port;
        Mux* mux;
        if (socket->getAddressFamily() == AF_INET)
        {
            switch (socket->getSocketType())
            {
            case ISocket::Stream:
                port = &tcpLast;
                mux = &tcpLocalPortMux;
                break;
            case ISocket::Datagram:
                port = &udpLast;
                mux = &udpLocalPortMux;
                break;
            default:
                return 0;
                break;
            }
        }
        // cf. http://www.iana.org/assignments/port-numbers
        int anon = *port;
        for (int i = 0; i <= 65535 - 49152; ++i)
        {
            if (65535 < anon)
            {
                anon = 49152;
            }
            if (!mux->contains(reinterpret_cast<void*>(anon)))
            {
                *port = anon + 1;
                return anon;
            }
            ++anon;
        }
        return 0;
    }

    IInternetAddress* selectSourceAddress(IInternetAddress* dst)
    {
        return selectSourceAddress(dynamic_cast<Inet4Address*>(dst));
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

    void joinGroup(Inet4Address* addr);
    void leaveGroup(Inet4Address* addr);

    Inet4Address* onLink(InAddr addr, int scopeID = 0);

    Inet4Address* getNextHop(Inet4Address* dst);
    Inet4Address* selectSourceAddress(Inet4Address* dst);

    bool isReachable(Inet4Address* address, long long timeout);

    // Inet4Address
    Inet4Address                addressAny;         // InAddrAny
    Inet4Address                addressAllRouters;  // InAddrAny

    friend class Inet4Address;
    friend class Inet4Address::StateInit;
    friend class Inet4Address::StateTentative;
    friend class Inet4Address::StatePreferred;
    friend class Inet4Address::StateReachable;
    friend class InReceiver;
};

#endif  // INET4_H_INCLUDED
