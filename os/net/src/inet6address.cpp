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
getAddress(void* address, int len)
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

const char* Inet6Address::
getCanonicalHostName(void* hostName, int len)
{
    return 0;
}

const char* Inet6Address::
getHostAddress(void* hostAddress, int len)
{
    return 0;
}

const char* Inet6Address::
getHostName(void* hostName, int len)
{
    return 0;
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

es::InternetAddress* Inet6Address::
getNext()
{
    return 0;
}

Object* Inet6Address::
socket(int type, int protocol)
{
    return 0;
}

// IInterface
Object* Inet6Address::
queryInterface(const char* riid)
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
