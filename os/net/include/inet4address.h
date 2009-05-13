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
        virtual bool isDeprecated()
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
        bool isDeprecated()
        {
            return true;
        }
        void start(Inet4Address* a);
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
    Inet4Address(InAddr addr, State& state, int scopeID = 0, int prefix = 0);
    virtual ~Inet4Address();

    State* getState()
    {
        return state;
    }

    void setState(State& state);

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
    int getAddress(void* address, int len);
    int getAddressFamily();
    const char* getCanonicalHostName(void* hostName, int len);
    const char* getHostAddress(void* hostAddress, int len);
    const char* getHostName(void* hostName, int len);

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
    bool isDeprecated()
    {
        return state->isDeprecated();
    }

    int getPathMTU()
    {
        return pathMTU;
    }

    void setPathMTU(int mtu)
    {
        pathMTU = std::max(68, mtu);
    }

    es::InternetAddress* getNext();

    Object* socket(int type, int protocol, int port);

    // IInterface
    Object* queryInterface(const char* riid);
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
