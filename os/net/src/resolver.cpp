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

#include "resolver.h"

IInternetAddress* Resolver::
getHostByName(const char* hostName, int addressFamily)
{
    return 0;
}

// Note DNS is not queried.
IInternetAddress* Resolver::
getHostByAddress(const void* address, unsigned int len, unsigned int scopeID)
{
    if (len == sizeof(InAddr))  // AF_INET
    {
        InAddr addr;
        memmove(&addr.addr, address, sizeof(InAddr));

        InFamily* inFamily = dynamic_cast<InFamily*>(Socket::getAddressFamily(AF_INET));
        Inet4Address* host = inFamily->getAddress(addr, scopeID);
        if (!host)
        {
            Handle<Inet4Address> onLink;
            if (IN_IS_ADDR_LOOPBACK(addr))
            {
                host = new Inet4Address(addr, Inet4Address::statePreferred, 1, 8);
            }
            else if (IN_IS_ADDR_MULTICAST(addr))
            {
                host = new Inet4Address(addr, Inet4Address::stateNonMember, scopeID);
            }
            else if (onLink = inFamily->onLink(addr, scopeID))
            {
                host = new Inet4Address(addr, Inet4Address::stateInit, onLink->getScopeID());
            }
            else
            {
                host = new Inet4Address(addr, Inet4Address::stateDestination, scopeID);
            }
            inFamily->addAddress(host);
        }
        return host;
    }

    if (len == sizeof(In6Addr)) // AF_INET6
    {
        return 0;
    }

    return 0;
}

bool Resolver::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IResolver)
    {
        *objectPtr = static_cast<IResolver*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IResolver*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Resolver::
addRef()
{
    return ref.addRef();
}

unsigned int Resolver::
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
