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

#include <string.h>
#include <es.h>
#include <es/dateTime.h>
#include <es/endian.h>
#include <es/handle.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/base/IService.h>
#include <es/base/IStream.h>
#include <es/base/IThread.h>
#include <es/device/INetworkInterface.h>
#include <es/naming/IContext.h>
#include <es/net/ISocket.h>
#include <es/net/IInternetConfig.h>
#include <es/net/IResolver.h>
#include <es/net/arp.h>
#include <es/net/dhcp.h>
#include <es/net/dns.h>
#include <es/net/udp.h>
#include "socket.h"

const InAddr DHCPHdr::magicCookie = { htonl(99 << 24 | 130 << 16 | 83 << 8 | 99) };

extern int esInit(IInterface** nameSpace);
extern IThread* esCreateThread(void* (*start)(void* param), void* param);
extern void esRegisterInternetProtocol(IContext* context);

static const int MaxDomainName = 256;
static const int MaxStaticRoute = 1;

struct DHCPStaticRoute
{
    InAddr  dest;
    InAddr  router;
};

struct DHCPInfo
{
    InAddr  ipaddr;                 // dhcp->yiaddr

    // BOOTP options
    InAddr  netmask;                // DHCPOption::SubnetMask
    InAddr  router;                 // DHCPOption::Router (1st)
    InAddr  dns[2];                 // DHCPOption::DomainNameServer
    char    host[MaxDomainName];    // DHCPOption::HostName
    char    domain[MaxDomainName];  // DHCPOption::DomainName
    u8      ttl;                    // DHCPOption::DefaultTTL
    u16     mtu;                    // DHCPOption::InterfaceMTU
    InAddr  broadcast;              // DHCPOption::BroadcastAddress

    // DHCP Extensions
    u32     lease;                  // DHCPOption::LeaseTime [sec]
    InAddr  server;                 // DHCPOption::ServerID
    u32     renewal;                // DHCPOption::RenewalTime [sec] (t1)
    u32     rebinding;              // DHCPOption::RebindingTime [sec] (t2)

    DHCPStaticRoute staticRoute[MaxStaticRoute];    // DHCPOption::StaticRoute
};

class DHCPControl : public IService
{
    static const int MinWait = 4;   // [sec] -1 to +1
    static const int MaxWait = 64;  // [sec]
    static const int MaxRxmit = 4;

    Ref                         ref;

    Handle<IResolver>           resolver;
    Handle<IInternetConfig>     config;
    int                         scopeID;
    u8                          chaddr[16]; // Client hardware address

    Handle<ISocket>             socket;
    Handle<IInternetAddress>    host;
    Handle<IInternetAddress>    server;
    Handle<IInternetAddress>    limited;

    volatile bool               enabled;

    int         state;
    u32         xid;
    u32         flag;

    u8          heap[IP_MIN_MTU - sizeof(IPHdr) - sizeof(UDPHdr)];

    int         rxmitMax;
    int         rxmitCount;

    DateTime    epoch;

    DHCPInfo    info;
    DHCPInfo    tentative;

    char        hostName[MaxDomainName];

    u8* requestList(u8* vend)
    {
        u8* ptr = vend;

        // Request list
        *ptr++ = DHCPOption::RequestList;
        *ptr++ = 2; // length
        *ptr++ = DHCPOption::SubnetMask;
        *ptr++ = DHCPOption::Router;
        *ptr++ = DHCPOption::DomainNameServer;
        *ptr++ = DHCPOption::HostName;
        *ptr++ = DHCPOption::DomainName;
        *ptr++ = DHCPOption::BroadcastAddress;
        *ptr++ = DHCPOption::StaticRoute;
        *ptr++ = DHCPOption::RenewalTime;
        *ptr++ = DHCPOption::RebindingTime;
        vend[1] = (u8) (ptr - vend - 2);
        return ptr;
    }

    int requestDiscover()
    {
        DHCPHdr* dhcp = reinterpret_cast<DHCPHdr*>(heap);
        dhcp->op = DHCPHdr::Request;
        dhcp->htype = 1;        // Ethernet
        dhcp->hlen = 6;         // Ethernet
        dhcp->hops = 0;
        dhcp->xid = htonl(xid);
        dhcp->secs = 0;
        dhcp->flags = 0;
        dhcp->ciaddr = dhcp->yiaddr = dhcp->siaddr = dhcp->giaddr = InAddrAny;
        memmove(dhcp->chaddr, chaddr, sizeof chaddr);
        memset(dhcp->sname, 0, 64);
        memset(dhcp->file,  0, 128);

        dhcp->cookie = DHCPHdr::magicCookie;

        u8* vend = heap + sizeof(DHCPHdr);

        *vend++ = DHCPOption::Type;
        *vend++ = 1;
        *vend++ = DHCPType::Discover;

        vend = requestList(vend);

        *vend++ = DHCPOption::End;
        int len = vend - heap;
        if (len < DHCPHdr::BootpHdrSize)
        {
            memset(vend, DHCPOption::Pad, DHCPHdr::BootpHdrSize - len);
            len = DHCPHdr::BootpHdrSize;
        }

        return len;
    }

    int requestRequest()
    {
        DHCPHdr* dhcp = reinterpret_cast<DHCPHdr*>(heap);
        dhcp->op = DHCPHdr::Request;
        dhcp->htype = 1;        // Ethernet
        dhcp->hlen = 6;         // Ethernet
        dhcp->hops = 0;
        dhcp->xid = htonl(xid);
        dhcp->secs = 0;
        dhcp->flags = 0;
        if (state == DHCPState::Renewing || state == DHCPState::Rebinding)
        {
            dhcp->ciaddr = info.ipaddr;
        }
        else
        {
            dhcp->ciaddr = InAddrAny;
        }
        dhcp->yiaddr = dhcp->siaddr = dhcp->giaddr = InAddrAny;
        memmove(dhcp->chaddr, chaddr, sizeof chaddr);
        memset(dhcp->sname, 0, 64);
        memset(dhcp->file,  0, 128);

        dhcp->cookie = DHCPHdr::magicCookie;

        u8* vend = heap + sizeof(DHCPHdr);

        *vend++ = DHCPOption::Type;
        *vend++ = 1;
        *vend++ = DHCPType::Request;

        vend = requestList(vend);

        // Server Identifier
        if (state == DHCPState::Requesting)
        {
            *vend++ = DHCPOption::ServerID;
            *vend++ = sizeof(InAddr);
            memmove(vend, &info.server, sizeof(InAddr));
            vend += sizeof(InAddr);
        }

        // Requested IP Address
        if (state == DHCPState::Requesting || state == DHCPState::Rebooting)
        {
            *vend++ = DHCPOption::RequestedAddress;
            *vend++ = sizeof(InAddr);
            memmove(vend, &info.ipaddr, sizeof(InAddr));
            vend += sizeof(InAddr);
        }

        *vend++ = DHCPOption::End;
        int len = vend - heap;
        if (len < DHCPHdr::BootpHdrSize)
        {
            memset(vend, DHCPOption::Pad, DHCPHdr::BootpHdrSize - len);
            len = DHCPHdr::BootpHdrSize;
        }

        return len;
    }

    int requestRelease()
    {
        DHCPHdr* dhcp = reinterpret_cast<DHCPHdr*>(heap);
        dhcp->op = DHCPHdr::Request;
        dhcp->htype = 1;        // Ethernet
        dhcp->hlen = 6;         // Ethernet
        dhcp->hops = 0;
        dhcp->xid = htonl(xid);
        dhcp->secs = 0;
        dhcp->flags = 0;
        dhcp->ciaddr = info.ipaddr;
        dhcp->yiaddr = dhcp->siaddr = dhcp->giaddr = InAddrAny;
        memmove(dhcp->chaddr, chaddr, sizeof chaddr);
        memset(dhcp->sname, 0, 64);
        memset(dhcp->file,  0, 128);

        dhcp->cookie = DHCPHdr::magicCookie;

        u8* vend = heap + sizeof(DHCPHdr);

        *vend++ = DHCPOption::Type;
        *vend++ = 1;
        *vend++ = DHCPType::Release;

        // Server Identifier
        *vend++ = DHCPOption::ServerID;
        *vend++ = sizeof(InAddr);
        memmove(vend, &info.server, sizeof(InAddr));
        vend += sizeof(InAddr);

        *vend++ = DHCPOption::End;
        int len = vend - heap;
        if (len < DHCPHdr::BootpHdrSize)
        {
            memset(vend, DHCPOption::Pad, DHCPHdr::BootpHdrSize - len);
            len = DHCPHdr::BootpHdrSize;
        }

        return len;
    }

    u8 reply(int len)
    {
        if (len < sizeof(DHCPHdr))
        {
            return 0;
        }
        DHCPHdr* dhcp = reinterpret_cast<DHCPHdr*>(heap);
        if (dhcp->op != DHCPHdr::Reply ||
            dhcp->htype != 1 ||
            dhcp->hlen != 6 ||
            dhcp->xid != htonl(xid) ||
            dhcp->cookie != DHCPHdr::magicCookie)
        {
            return 0;
        }
        if (sizeof heap < len)
        {
            len = sizeof heap;
        }

        u8 messageType = 0;
        memset(&tentative, 0, sizeof(DHCPInfo));

        bool overloaded = false;
        u8* sname = 0;
        u8* file  = 0;
        u8  ttl;
        u16 mtu;

        u8* vend = heap + sizeof(DHCPHdr);
        int optlen = len - sizeof(sizeof(DHCPHdr));

    Overload:
        while (0 < optlen && *vend != DHCPOption::End)
        {
            u8 type = *vend++;
            --optlen;
            switch (type)
            {
            case DHCPOption::Pad:
                len = 1;
                break;
            default:
                if (optlen < 1)
                {
                    return 0;
                }
                len = *vend++;
                --optlen;
                if (optlen < len)
                {
                    return 0;
                }
                break;
            }

            switch (type)
            {
            //
            // BOOTP options
            //
            case DHCPOption::SubnetMask:
                if (len != sizeof(InAddr))
                {
                    return 0;
                }
                tentative.netmask = *reinterpret_cast<InAddr*>(vend);
                break;
            case DHCPOption::Router:
                if (len <= 0 || len % sizeof(InAddr))
                {
                    return 0;
                }
                // Choose the 1st one
                tentative.router = *reinterpret_cast<InAddr*>(vend);
                break;
            case DHCPOption::DomainNameServer:
                if (len <= 0 || len % sizeof(InAddr))
                {
                    return 0;
                }
                // Choose the 1st two
                tentative.dns[0] = *reinterpret_cast<InAddr*>(vend);
                if (8 <= len)
                {
                    tentative.dns[1] = *reinterpret_cast<InAddr*>(vend + sizeof(InAddr));
                }
                break;
            case DHCPOption::HostName:
                memmove(tentative.host, vend, len);
                tentative.host[len] = '\0';
                break;
            case DHCPOption::DomainName:
                memmove(tentative.domain, vend, len);
                tentative.domain[len] = '\0';
                break;
            case DHCPOption::DefaultTTL:
                ttl = *(u8*) vend;
                if (0 < ttl)
                {
                    tentative.ttl = ttl;
                }
                break;
            case DHCPOption::InterfaceMTU:
                mtu = ntohs(*(u16*) vend);
                if (68 <= mtu)
                {
                    tentative.mtu = mtu;
                }
                break;

            case DHCPOption::BroadcastAddress:
                if (len != sizeof(InAddr))
                {
                    return 0;
                }
                tentative.broadcast = *reinterpret_cast<InAddr*>(vend);
                break;

            case DHCPOption::StaticRoute:
                if (len <= 0 || len % sizeof(DHCPStaticRoute))
                {
                    return 0;
                }
                memmove(tentative.staticRoute, vend, std::min((int) (MaxStaticRoute * sizeof(DHCPStaticRoute)), len));
                break;

            //
            // DHCP Extensions
            //
            case DHCPOption::LeaseTime:
                if (len != 4)
                {
                    return 0;
                }
                tentative.lease = ntohl(*(u32*) vend);
                break;
            case DHCPOption::Overload:
                if (len != 1 || overloaded)
                {
                    return 0;
                }
                overloaded = true;
                switch (*vend)
                {
                  case DHCPHdr::File:
                    file = dhcp->file;
                    break;
                  case DHCPHdr::Sname:
                    sname = dhcp->sname;
                    break;
                  case DHCPHdr::FileAndSname:
                    file = dhcp->file;
                    sname = dhcp->sname;
                    break;
                }
                break;
            case DHCPOption::Type:
                if (len != 1)
                {
                    return 0;
                }
                messageType = *vend;
                break;
            case DHCPOption::ServerID:
                if (len != sizeof(InAddr))
                {
                    return 0;
                }
                tentative.server = *reinterpret_cast<InAddr*>(vend);
                break;
            case DHCPOption::RenewalTime:
                if (len != 4)
                {
                    return 0;
                }
                tentative.renewal = ntohl(*(u32*) vend);
                break;
            case DHCPOption::RebindingTime:
                if (len != 4)
                {
                    return 0;
                }
                tentative.rebinding = ntohl(*(u32*) vend);
                break;
            default:
                break;
            }

            vend += len;
            optlen -= len;
        }

        if (sname)
        {
            vend = sname;
            optlen = sizeof(dhcp->sname);
            sname = 0;
            goto Overload;
        }

        if (file)
        {
            vend = file;
            optlen = sizeof(dhcp->file);
            file = 0;
            goto Overload;
        }

        switch (messageType)
        {
        case DHCPType::Offer:
        case DHCPType::Ack:
            // Check dhcp->yiaddr
            if (IN_IS_ADDR_MULTICAST(dhcp->yiaddr) ||
                IN_IS_ADDR_RESERVED(dhcp->yiaddr) ||
                IN_ARE_ADDR_EQUAL(dhcp->yiaddr, InAddrAny) ||
                IN_ARE_ADDR_EQUAL(dhcp->yiaddr, InAddrLoopback) ||
                IN_ARE_ADDR_EQUAL(dhcp->yiaddr, InAddrBroadcast))
            {
                return 0;
            }
            break;
        }

        tentative.ipaddr = dhcp->yiaddr;

        if (tentative.renewal == 0 || tentative.lease <= tentative.renewal)
        {
            // T1 defaults to (0.5 * duration_of_lease)
            tentative.renewal = tentative.lease / 2;
        }

        if (tentative.rebinding == 0 || tentative.lease <= tentative.rebinding)
        {
            // T2 defaults to (0.875 * duration_of_lease)
            tentative.rebinding = (u32) ((7 * (u64) tentative.lease) / 8);
        }
        if (tentative.rebinding <= tentative.renewal)
        {
            tentative.renewal = (u32) ((4 * (u64) tentative.rebinding) / 7);
        }

        return messageType;
    }

    bool sleep(DateTime till)
    {
        DateTime now = DateTime::getNow();
        while (enabled && now < till)
        {
            socket->setTimeout(till - now);
            socket->read(heap, sizeof heap);
            now = DateTime::getNow();
        }
        return enabled;
    }

public:
    DHCPControl(IContext* context, int scopeID, u8 chaddr[16]) :
        scopeID(scopeID),
        enabled(false),
        state(DHCPState::Init),
        xid(0),
        flag(0),
        rxmitMax(MaxRxmit),
        rxmitCount(0)
    {
        memmove(this->chaddr, chaddr, sizeof this->chaddr);

        resolver = context->lookup("network/resolver");
        config = context->lookup("network/config");

        limited = resolver->getHostByAddress(&InAddrBroadcast.addr, sizeof(InAddr), scopeID);
    }

    ~DHCPControl()
    {
        socket = 0;
    }

    void run()
    {
        enabled = true;

        xid = (u32) DateTime::getNow().getTicks();
        xid ^= *(u32*) chaddr;

        Handle<IInternetAddress> any = resolver->getHostByAddress(&InAddrAny.addr, sizeof(InAddr), scopeID);
        socket = any->socket(AF_INET, ISocket::Datagram, DHCPHdr::ClientPort);
        u8 type = 0;

        // Send DHCPDISCOVER
        state = DHCPState::Selecting;
        for (rxmitCount = 0; enabled && rxmitCount < rxmitMax; ++rxmitCount)
        {
            int len = requestDiscover();
            socket->sendTo(heap, len, 0, limited, DHCPHdr::ServerPort);
            socket->setTimeout(TimeSpan(0, 0, MinWait << rxmitCount));
            len = socket->read(heap, sizeof heap);
            type = reply(len);
            if (type == DHCPType::Offer)
            {
                break;
            }
        }

        if (type != DHCPType::Offer)
        {
            esReport("DHCP: failed.\n");
            state = DHCPState::Init;
            socket->close();
            return;
        }

        // Send DHCPREQUEST
        info = tentative;
        state = DHCPState::Requesting;
        DateTime now = DateTime::getNow();
        for (rxmitCount = 0; enabled && rxmitCount < rxmitMax; ++rxmitCount)
        {
            int len = requestRequest();
            socket->sendTo(heap, len, 0, limited, DHCPHdr::ServerPort);
            socket->setTimeout(TimeSpan(0, 0, MinWait << rxmitCount));
            do
            {
                len = socket->read(heap, sizeof heap);
                type = reply(len);
            } while (type == DHCPType::Offer);
            if (type == DHCPType::Ack || type == DHCPType::Nak)
            {
                break;
            }
        }

        if (type != DHCPType::Ack || tentative.lease == 0)
        {
            esReport("DHCP: failed.\n");
            state = DHCPState::Init;
            socket->close();
            return;
        }

        info = tentative;
        state = DHCPState::Bound;

        unsigned int prefix = 32 - (ffs(ntohl(info.netmask.addr)) - 1);

        esReport("lease %u\n", info.lease);
        esReport("renewal %u\n", info.renewal);
        esReport("rebinding: %u\n", info.rebinding);
        esReport("prefix: %u\n", prefix);

        // Register host address (info.ipaddr)
        host = resolver->getHostByAddress(&info.ipaddr.addr, sizeof(InAddr), scopeID);
        config->addAddress(host, prefix);

        if (!sleep(DateTime::getNow() + 90000000))  // Wait for the host address to be settled.
        {
            config->removeAddress(host);
            socket->close();
            return;
        }
        // XXX Send decline if configuration was failed.

        // Register a default router (info.router)
        Handle<IInternetAddress> router;
        if (!IN_IS_ADDR_UNSPECIFIED(info.router))
        {
            router = resolver->getHostByAddress(&info.router.addr, sizeof(InAddr), scopeID);
            config->addRouter(router);
        }

        // Register a domain name server (info.dns[0])
        Handle<IInternetAddress> nameServer;
        if (!IN_IS_ADDR_UNSPECIFIED(info.dns[0]))
        {
            nameServer = resolver->getHostByAddress(&info.dns[0].addr, sizeof(InAddr), 0);
            config->addNameServer(nameServer);
        }

        socket->close();
        socket = host->socket(AF_INET, ISocket::Datagram, DHCPHdr::ClientPort);
        server = resolver->getHostByAddress(&info.server, sizeof(InAddr), scopeID);

        while (enabled)
        {
            state = DHCPState::Bound;
            epoch = now;

            // Wait for T1 to move to RENEWING state.
            if (!sleep(epoch + TimeSpan(0, 0, info.renewal)))
            {
                continue;
            }

            state = DHCPState::Renewing;
            type = 0;
            while (enabled && (now = DateTime::getNow()) < epoch + TimeSpan(0, 0, info.rebinding))
            {
                int len = requestRequest();
                socket->sendTo(heap, len, 0, server, DHCPHdr::ServerPort);
                TimeSpan wait = (epoch + TimeSpan(0, 0, info.rebinding) - now) / 2;
                if (wait < TimeSpan(0, 0, 60))
                {
                    break;
                }
                socket->setTimeout(wait);
                len = socket->read(heap, sizeof heap);
                type = reply(len);
                if (type == DHCPType::Ack || type == DHCPType::Nak)
                {
                    break;
                }
            }

            if (type == DHCPType::Nak)
            {
                esReport("DHCP: failed renewing.\n");
                state = DHCPState::Init;
                break;
            }

            if (type == DHCPType::Ack)
            {
                continue;
            }

            // Wait for T2 to move to REBINDING state.
            if (!sleep(epoch + TimeSpan(0, 0, info.rebinding)))
            {
                continue;
            }

            state = DHCPState::Rebinding;
            type = 0;
            while (enabled && (now = DateTime::getNow()) < epoch + TimeSpan(0, 0, info.rebinding))
            {
                int len = requestRequest();
                socket->sendTo(heap, len, 0, limited, DHCPHdr::ServerPort);
                TimeSpan wait = (epoch + TimeSpan(0, 0, info.lease) - now) / 2;
                if (wait < TimeSpan(0, 0, 60))
                {
                    break;
                }
                socket->setTimeout(wait);
                len = socket->read(heap, sizeof heap);
                type = reply(len);
                if (type == DHCPType::Ack || type == DHCPType::Nak)
                {
                    break;
                }
            }

            if (type == DHCPType::Nak)
            {
                esReport("DHCP: failed renewing.\n");
                state = DHCPState::Init;
                break;
            }

            if (type != DHCPType::Ack)
            {
                esReport("DHCP: failed rebinding.\n");
                break;
            }
        }

        if (state == DHCPState::Bound ||
            state == DHCPState::Renewing ||
            state == DHCPState::Rebinding)
        {
            int len = requestRelease();
            socket->sendTo(heap, len, 0, server, DHCPHdr::ServerPort);
            sleep(DateTime::getNow() + 10000000);
        }

        socket->close();
        socket = 0;

        config->removeAddress(host);
        if (router)
        {
            config->removeRouter(router);
        }
        state = DHCPState::Init;

        host = 0;
        server = 0;
        router = 0;
    }

    bool start()
    {
        if (!enabled)
        {
            IThread* thread = esCreateThread(run, this);
            thread->start();
            thread->setPriority(IThread::Highest - 2);
            thread->release();
        }
        return enabled;
    }

    bool stop()
    {
        if (enabled)
        {
            enabled = false;
            socket->notify();
            esSleep(100000000);
        }
        return enabled;
    }

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IService)
        {
            *objectPtr = static_cast<IService*>(this);
        }
        else if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IService*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
    }

    unsigned int addRef(void)
    {
        return ref.addRef();
    }

    unsigned int release(void)
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }

    static void* run(void* param)
    {
        // Test listen and accept operations
        DHCPControl* dhcp = static_cast<DHCPControl*>(param);
        dhcp->run();
        return 0;
    }
};

void esRegisterDHCPClient(IContext* context)
{
    for (int scopeID = 1; scopeID < Socket::INTERFACE_MAX; ++scopeID)
    {
        char path[48];
        sprintf(path, "network/interface/%u/interface", scopeID);

        Handle<INetworkInterface> nic = context->lookup(path);
        if (!nic || nic->getType() == INetworkInterface::Loopback)
        {
            continue;
        }

        u8 chaddr[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        nic->getMacAddress(chaddr);
        DHCPControl* dhcp = new DHCPControl(context, scopeID, chaddr);

        sprintf(path, "network/interface/%u/dhcp", scopeID);
        esReport("esRegisterDHCPClient: %s\n", path);
        Handle<IBinding> binding = context->bind(path, dhcp);
    }
}
