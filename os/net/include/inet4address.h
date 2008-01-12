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

#ifndef INET4ADDRESS_H_INCLUDED
#define INET4ADDRESS_H_INCLUDED

#include <algorithm>
#include <es/collection.h>
#include <es/endian.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/timer.h>
#include <es/timeSpan.h>
#include <es/net/inet4.h>
#include "address.h"
#include "inet.h"

class InFamily;
class Socket;

class Inet4Address :
    public Address,
    public InetReceiver,
    public TimerTask
{
    class State
    {
    public:
        virtual bool isLocalAddress()
        {
            return false;
        }
        virtual bool isPreferred()
        {
            return false;
        }

        virtual void start(Inet4Address* a)
        {
        }

        virtual void stop(Inet4Address* a)
        {
        }

        virtual void expired(Inet4Address* a)
        {
        }

        virtual bool input(InetMessenger* m, Inet4Address* a) = 0;
        virtual bool output(InetMessenger* m, Inet4Address* a) = 0;
        virtual bool error(InetMessenger* m, Inet4Address* a) = 0;
    };

    // The following for states are used for ARP cache. [RFC 826]
    class StateInit : public State
    {
    public:
        void start(Inet4Address* a);
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    class StateIncomplete : public State
    {
    public:
        void expired(Inet4Address* a);
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    class StateReachable : public State
    {
    public:
        void start(Inet4Address* a);
        void expired(Inet4Address* a);
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    class StateProbe : public State
    {
    public:
        void expired(Inet4Address* a);
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    // The following three states are used for address configuration. [RFC 3927]
    class StateTentative : public State
    {
    public:
        bool isLocalAddress()
        {
            return true;
        }
        void start(Inet4Address* a);
        void expired(Inet4Address* a);
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    class StatePreferred : public State
    {
    public:
        bool isLocalAddress()
        {
            return true;
        }
        bool isPreferred()
        {
            return true;
        }
        void start(Inet4Address* a);
        void expired(Inet4Address* a);
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    class StateDeprecated : public State
    {
    public:
        bool isLocalAddress()
        {
            return true;
        }
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    // The following three states are used for IGMP. [RFC 1112, 2236, 3376]
    class StateNonMember : public State
    {
    public:
        void start(Inet4Address* a);
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    class StateDelayingMember : public State
    {
    public:
        void stop(Inet4Address* a);
        void expired(Inet4Address* a);
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    class StateIdleMember : public State
    {
    public:
        void stop(Inet4Address* a);
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    // The following state is used for ICMP redirect. [RFC 792]
    class StateDestination : public State
    {
    public:
        void start(Inet4Address* a);
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    // The following state is used for on-link "Prefix List".  [RFC 2461]
    class StatePrefix : public State
    {
    public:
        bool input(InetMessenger* m, Inet4Address* a);
        bool output(InetMessenger* m, Inet4Address* a);
        bool error(InetMessenger* m, Inet4Address* a);
    };

    Ref         ref;
    State*      state;
    InAddr      addr;
    int         scopeID;
    int         prefix;
    InFamily*   inFamily;
    Conduit*    adapter;
    int         timeoutCount;
    int         pathMTU;

public:
    Inet4Address(InAddr addr, State& state, int scopeID = 0, int prefix = 0) :
        state(&state),
        addr(addr),
        scopeID(scopeID),
        prefix(prefix),
        inFamily(0),
        adapter(0),
        timeoutCount(0),
        pathMTU(1500)
    {
        ASSERT(0 <= prefix && prefix < 32);
        u8 mac[6];

        if (IN_IS_ADDR_MULTICAST(addr))
        {
            mac[0] = 0x01;
            mac[1] = 0x00;
            mac[2] = 0x5e;
            memmove(&mac[3], &reinterpret_cast<u8*>(&addr)[1], 3);
            mac[3] &= 0x7f;
            setMacAddress(mac);
        }
        else if (IN_ARE_ADDR_EQUAL(addr, InAddrBroadcast))  // XXX or directed mcast
        {
            memset(mac, 0xff, 6);
            setMacAddress(mac);
        }
    }

    virtual ~Inet4Address()
    {
    }

    State* getState()
    {
        return state;
    }

    void setState(State& state)
    {
        timeoutCount = 0;   // Reset timeout count
        this->state = &state;
    }

    s32 sumUp()
    {
        return (addr.addr >> 16) + (addr.addr & 0xffff);
    }

    Address* getNextHop();

    InAddr getAddress()
    {
        return addr;
    }

    int getPrefix()
    {
        return prefix;
    }
    void setPrefix(int prefix)
    {
        this->prefix = prefix;
    }

    InAddr getMask()
    {
        InAddr mask;
        mask.addr = htonl(0xffffffffu << (32 - prefix));
        return mask;
    }

    Inet4Address* clone(Conduit* conduit, void* key)
    {
        Inet4Address* address = static_cast<Inet4Address*>(key);
        if (address)
        {
            address->adapter = conduit;
            address->addRef();
        }
        return address;
    }

    Conduit* getAdapter() const
    {
        return adapter;
    }

    void start()
    {
        return state->start(this);
    }

    void stop()
    {
        return state->stop(this);
    }

    bool input(InetMessenger* m, Conduit* c)
    {
        return state->input(m, this);
    }

    bool output(InetMessenger* m, Conduit* c)
    {
        return state->output(m, this);
    }

    bool error(InetMessenger* m, Conduit* c)
    {
        return state->error(m, this);
    }

    void run()
    {
        state->expired(this);
    }

    void alarm(TimeSpan delay);
    void cancel();

    // IInternetAddress
    int getAddress(void* address, unsigned int len);
    int getAddressFamily();
    int getCanonicalHostName(char* hostName, unsigned int len);
    int getHostAddress(char* hostAddress, unsigned int len);
    int getHostName(char* hostName, unsigned int len);

    int getScopeID()
    {
        return scopeID;
    }
    void setScopeID(int id);

    bool isUnspecified()
    {
        return IN_IS_ADDR_UNSPECIFIED(addr);
    }
    bool isLinkLocal()
    {
        return IN_IS_ADDR_LINKLOCAL(addr);
    }
    bool isLoopback()
    {
        return IN_IS_ADDR_LOOPBACK(addr);
    }
    bool isMulticast()
    {
        return IN_IS_ADDR_MULTICAST(addr);
    }

    bool isReachable(long long timeout);

    bool isLocalAddress()
    {
        return state->isLocalAddress();
    }
    bool isPreferred()
    {
        return state->isPreferred();
    }

    int getPathMTU()
    {
        return pathMTU;
    }

    void setPathMTU(int mtu)
    {
        pathMTU = std::max(68, mtu);
    }

    IInternetAddress* getNext();

    IInterface* socket(int type, int protocol, int port);

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef();
    unsigned int release();

    static StateInit            stateInit;
    static StateIncomplete      stateIncomplete;
    static StateReachable       stateReachable;
    static StateProbe           stateProbe;
    static StateTentative       stateTentative;
    static StatePreferred       statePreferred;
    static StateDeprecated      stateDeprecated;
    static StateNonMember       stateNonMember;
    static StateDelayingMember  stateDelayingMember;
    static StateIdleMember      stateIdleMember;
    static StateDestination     stateDestination;
    static StatePrefix          statePrefix;

    friend class InFamily;
};

#endif  // INET4ADDRESS_H_INCLUDED
