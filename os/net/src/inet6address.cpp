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

#include <new>
#include "inet6address.h"
#include "socket.h"

Inet6Address::StateInit             Inet6Address::stateInit;
Inet6Address::StateIncomplete       Inet6Address::stateIncomplete;
Inet6Address::StateReachable        Inet6Address::stateReachable;
Inet6Address::StateStale            Inet6Address::stateStale;
Inet6Address::StateDelay            Inet6Address::stateDelay;
Inet6Address::StateProbe            Inet6Address::stateProbe;
Inet6Address::StateTentative        Inet6Address::stateTentative;
Inet6Address::StatePreferred        Inet6Address::statePreferred;
Inet6Address::StateDeprecated       Inet6Address::stateDeprecated;
Inet6Address::StateNonListener      Inet6Address::stateNonListener;
Inet6Address::StateDelayingListener Inet6Address::stateDelayingListener;
Inet6Address::StateIdleListener     Inet6Address::stateIdleListener;
Inet6Address::StateDestination      Inet6Address::stateDestination;
Inet6Address::StatePrefix           Inet6Address::statePrefix;

Inet6Address::
Inet6Address() :
    state(stateInit),
    addr(In6AddrAny)
{
}

Inet6Address::
~Inet6Address()
{
}

// IInternetAddress
int Inet6Address::
getAddress(void* address, unsigned int len)
{
    if (sizeof(addr) <= len)
    {
        memmove(address, &addr, sizeof(addr));
        return len;
    }
    return 0;
}

int Inet6Address::
getAddressFamily()
{
    return AF_INET6;
}

int Inet6Address::
getCanonicalHostName(char* hostName, unsigned int len)
{
}

int Inet6Address::
getHostAddress(char* hostAddress, unsigned int len)
{
}

int Inet6Address::
getHostName(char* hostName, unsigned int len)
{
}

int Inet6Address::
getScopeID()
{
    return scopeID;
}

bool Inet6Address::
isUnspecified()
{
}

bool Inet6Address::
isLinkLocal()
{
}

bool Inet6Address::
isLoopback()
{
}

bool Inet6Address::
isMulticast()
{
}

bool Inet6Address::
isReachable(long long timeout)
{
}

IInternetAddress* Inet6Address::
getNext()
{
    return 0;
}

IInterface* Inet6Address::
socket(int type, int protocol)
{
    return 0;
}

// IInterface
bool Inet6Address::
queryInterface(const Guid& riid, void** objectPtr)
{
}

unsigned int Inet6Address::
addRef()
{
}

unsigned int Inet6Address::
release()
{
}
