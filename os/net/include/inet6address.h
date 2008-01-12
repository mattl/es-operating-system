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

#ifndef INET6ADDRESS_H_INCLUDED
#define INET6ADDRESS_H_INCLUDED

#include <es/endian.h>
#include <es/net/inet6.h>
#include "address.h"
#include "inet.h"

class Inet6Address :
    public Address,
    public InetReceiver
{
    class State
    {
    public:
        virtual bool input(InetMessenger* m, Inet6Address* a) = 0;
        virtual bool output(InetMessenger* m, Inet6Address* a) = 0;
        virtual bool error(InetMessenger* m, Inet6Address* a) = 0;
    };

    // The following six states are used for "Neighbor Cache". [RFC 2461]
    class StateInit : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    class StateIncomplete : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    class StateReachable : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    class StateStale : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    class StateDelay : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    class StateProbe : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    // The following three states are used for address configuration. [RFC 2462]
    class StateTentative : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    class StatePreferred : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    class StateDeprecated : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    // The following three states are used for "Multicast Listener". [RFC 2710]
    class StateNonListener : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    class StateDelayingListener : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    class StateIdleListener : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    // The following state is used for "Destination Cache".  [RFC 2461]
    class StateDestination : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    // The following state is used for on-link "Prefix List".  [RFC 2461]
    class StatePrefix : public State
    {
    public:
        bool input(InetMessenger* m, Inet6Address* a);
        bool output(InetMessenger* m, Inet6Address* a);
        bool error(InetMessenger* m, Inet6Address* a);
    };

    In6Addr addr;
    State&  state;
    int     scopeID;

public:
    Inet6Address();
    virtual ~Inet6Address();

    State& getState()
    {
        return state;
    }
    void setState(State& state)
    {
        this->state = state;
    }

    bool input(InetMessenger* m, Conduit* c)
    {
        return state.input(m, this);
    }
    bool output(InetMessenger* m, Conduit* c)
    {
        return state.output(m, this);
    }
    bool error(InetMessenger* m, Conduit* c)
    {
        return state.error(m, this);
    }

    Address* getNextHop();

    // IInternetAddress
    int getAddress(void* address, unsigned int len);
    int getAddressFamily();
    int getCanonicalHostName(char* hostName, unsigned int len);
    int getHostAddress(char* hostAddress, unsigned int len);
    int getHostName(char* hostName, unsigned int len);
    int getScopeID();
    bool isUnspecified();
    bool isLinkLocal();
    bool isLoopback();
    bool isMulticast();
    bool isReachable(long long timeout);
    IInternetAddress* getNext();
    IInterface* socket(int type, int protocol);

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef();
    unsigned int release();

    static StateInit                stateInit;
    static StateIncomplete          stateIncomplete;
    static StateReachable           stateReachable;
    static StateStale               stateStale;
    static StateDelay               stateDelay;
    static StateProbe               stateProbe;
    static StateTentative           stateTentative;
    static StatePreferred           statePreferred;
    static StateDeprecated          stateDeprecated;
    static StateNonListener         stateNonListener;
    static StateDelayingListener    stateDelayingListener;
    static StateIdleListener        stateIdleListener;
    static StateDestination         stateDestination;
    static StatePrefix              statePrefix;
};

#endif  // INET6ADDRESS_H_INCLUDED
