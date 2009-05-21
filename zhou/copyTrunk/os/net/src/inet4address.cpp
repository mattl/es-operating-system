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

#include "inet4.h"
#include "inet4address.h"

Inet4Address::StateInit             Inet4Address::stateInit;
Inet4Address::StateIncomplete       Inet4Address::stateIncomplete;
Inet4Address::StateReachable        Inet4Address::stateReachable;
Inet4Address::StateProbe            Inet4Address::stateProbe;
Inet4Address::StateTentative        Inet4Address::stateTentative;
Inet4Address::StatePreferred        Inet4Address::statePreferred;
Inet4Address::StateDeprecated       Inet4Address::stateDeprecated;
Inet4Address::StateNonMember        Inet4Address::stateNonMember;
Inet4Address::StateDelayingMember   Inet4Address::stateDelayingMember;
Inet4Address::StateIdleMember       Inet4Address::stateIdleMember;
Inet4Address::StateDestination      Inet4Address::stateDestination;
Inet4Address::StatePrefix           Inet4Address::statePrefix;

Inet4Address::
Inet4Address(InAddr addr, State& state, int scopeID, int prefix) :
    state(&state),
    addr(addr),
    scopeID(scopeID),
    prefix(prefix),
    inFamily(0),
    adapter(0),
    timeoutCount(0),
    pathMTU(1500)
{
    ASSERT(0 <= prefix && prefix <= 32);
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

Inet4Address::
~Inet4Address()
{
    cancel();
    if (inFamily)
    {
        inFamily->removeAddress(this);
        inFamily = 0;
    }
}

void Inet4Address::
setState(State& state)
{
    timeoutCount = 0;   // Reset timeout count
    this->state = &state;
}

Address* Inet4Address::
getNextHop()
{
    if (state != &stateDestination && state != &statePrefix)
    {
        addRef();
        return this;
    }

    return inFamily->getNextHop(this);
}

void Inet4Address::
alarm(TimeSpan delay)
{
    Socket::alarm(this, delay);
}

void Inet4Address::
cancel()
{
    Socket::cancel(this);
}

// IInternetAddress
int Inet4Address::
getAddress(void* address, int len)
{
    if (sizeof(addr) <= len)
    {
        memmove(address, &addr, sizeof(addr));
        return len;
    }
    return 0;
}

int Inet4Address::
getAddressFamily()
{
    return AF_INET;
}

const char* Inet4Address::
getCanonicalHostName(void* hostName, int len)
{
    return 0;
}

const char* Inet4Address::
getHostAddress(void* hostAddress, int len)
{
    return 0;
}

const char* Inet4Address::
getHostName(void* hostName, int len)
{
    return Socket::resolver->getHostName(hostName, len, this);
}

void Inet4Address::
setScopeID(int id)
{
    if (id == scopeID)
    {
        return;
    }

    InFamily* family = inFamily;
    if (!family)
    {
        scopeID = id;
        return;
    }

    Handle<Inet4Address> address = inFamily->getAddress(addr, scopeID);
    if (address)
    {
        family->removeAddress(this);
        scopeID = id;
        family->addAddress(this);
    }
    else
    {
        scopeID = id;
    }
}

bool Inet4Address::
isReachable(long long timeout)
{
    return inFamily->isReachable(this, timeout);
}

es::InternetAddress* Inet4Address::
getNext()
{
    return 0;
}

Object* Inet4Address::
socket(int type, int protocol, int port)
{
    Socket* socket = new Socket(type, protocol);
    if (port == 0)
    {
        return static_cast<es::Socket*>(socket);
    }

    if (isLocalAddress())
    {
        socket->bind(this, port);
    }
    else if (isMulticast())
    {
        socket->bind(this, port);
        socket->joinGroup(this);
    }
    else
    {
        socket->connect(this, port);
    }
    return static_cast<es::Socket*>(socket);
}

// IInterface
Object* Inet4Address::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::InternetAddress::iid()) == 0)
    {
        objectPtr = static_cast<es::InternetAddress*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::InternetAddress*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Inet4Address::
addRef()
{
    return ref.addRef();
}

unsigned int Inet4Address::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}

// StateDestination

void Inet4Address::
StateDestination::start(Inet4Address* a)    // ARP for everything
{
    // Install ARP cache for this address.
    ASSERT(1 < a->getScopeID());
    a->inFamily->arpFamily.addAddress(a);
    a->setState(stateIncomplete);
    a->run();   // Invoke expired
}

bool Inet4Address::
StateDestination::input(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateDestination::output(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateDestination::error(InetMessenger* m, Inet4Address* a)
{
    return true;
}

// StatePrefix

bool Inet4Address::
StatePrefix::input(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StatePrefix::output(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StatePrefix::error(InetMessenger* m, Inet4Address* a)
{
    return true;
}
