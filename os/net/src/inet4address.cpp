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

int Inet4Address::
getCanonicalHostName(char* hostName, int len)
{
}

int Inet4Address::
getHostAddress(char* hostAddress, int len)
{
}

int Inet4Address::
getHostName(char* hostName, int len)
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

IInternetAddress* Inet4Address::
getNext()
{
    return 0;
}

IInterface* Inet4Address::
socket(int type, int protocol, int port)
{
    Socket* socket = new Socket(type, protocol);
    if (port == 0)
    {
        return static_cast<ISocket*>(socket);
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
    return static_cast<ISocket*>(socket);
}

// IInterface
void* Inet4Address::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == IInternetAddress::iid())
    {
        objectPtr = static_cast<IInternetAddress*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<IInternetAddress*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
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
