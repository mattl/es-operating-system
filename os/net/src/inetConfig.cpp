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

#include "inetConfig.h"

void InternetConfig::
addAddress(IInternetAddress* address, unsigned int prefix)
{
    int af = address->getAddressFamily();
    switch (af)
    {
    case AF_INET:
        Inet4Address* host = dynamic_cast<Inet4Address*>(address);
        if (host)
        {
            host->setPrefix(prefix);
            host->setState(Inet4Address::stateTentative);
            host->start();
        }
        break;
    }
}

IInternetAddress* InternetConfig::
getAddress(unsigned int scopeID)
{
    return 0;   // XXX
}

void InternetConfig::
removeAddress(IInternetAddress* address)
{
    int af = address->getAddressFamily();
    switch (af)
    {
    case AF_INET:
        Inet4Address* host = dynamic_cast<Inet4Address*>(address);
        if (host)
        {
            InFamily* inFamily = dynamic_cast<InFamily*>(Socket::getAddressFamily(AF_INET));
            inFamily->removeAddress(host);
        }
        break;
    }
}

void InternetConfig::
addRouter(IInternetAddress* router)
{
    int af = router->getAddressFamily();
    switch (af)
    {
    case AF_INET:
        Inet4Address* host = dynamic_cast<Inet4Address*>(router);
        if (host)
        {
            InFamily* inFamily = dynamic_cast<InFamily*>(Socket::getAddressFamily(AF_INET));
            inFamily->addRouter(host);
        }
        break;
    }
}

IInternetAddress* InternetConfig::getRouter()
{
    return 0;   // XXX
}

void InternetConfig::
removeRouter(IInternetAddress* router)
{
    int af = router->getAddressFamily();
    switch (af)
    {
    case AF_INET:
        Inet4Address* host = dynamic_cast<Inet4Address*>(router);
        if (host)
        {
            InFamily* inFamily = dynamic_cast<InFamily*>(Socket::getAddressFamily(AF_INET));
            inFamily->removeRouter(host);
        }
        break;
    }
}

int InternetConfig::addInterface(IStream* stream, int hrd)
{
    return Socket::addInterface(stream, hrd);
}

IInterface* InternetConfig::getInterface(int scopeID)
{
    Interface* interface = Socket::getInterface(scopeID);
    return interface->getStream();
}

void InternetConfig::removeInterface(IStream* stream)
{
    Socket::removeInterface(stream);
}

bool InternetConfig::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IInternetConfig)
    {
        *objectPtr = static_cast<IInternetConfig*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IInternetConfig*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int InternetConfig::
addRef()
{
    return ref.addRef();
}

unsigned int InternetConfig::
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
