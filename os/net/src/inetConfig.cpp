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

#include <es/naming/IBinding.h>
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
    InFamily* family = static_cast<InFamily*>(Socket::getAddressFamily(AF_INET));
    if (family)
    {
        return family->getHostAddress(scopeID);
    }
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
        if (host && host->isLocalAddress())
        {
            host->cancel();
            host->setState(Inet4Address::stateDeprecated);
            host->start();
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
        Inet4Address* node = dynamic_cast<Inet4Address*>(router);
        if (node)
        {
            InFamily* inFamily = dynamic_cast<InFamily*>(Socket::getAddressFamily(AF_INET));
            inFamily->addRouter(node);
        }
        break;
    }
}

IInternetAddress* InternetConfig::getRouter()
{
    InFamily* inFamily = dynamic_cast<InFamily*>(Socket::getAddressFamily(AF_INET));
    return inFamily->getRouter();
}

void InternetConfig::
removeRouter(IInternetAddress* router)
{
    int af = router->getAddressFamily();
    switch (af)
    {
    case AF_INET:
        Inet4Address* node = dynamic_cast<Inet4Address*>(router);
        if (node)
        {
            InFamily* inFamily = dynamic_cast<InFamily*>(Socket::getAddressFamily(AF_INET));
            inFamily->removeRouter(node);
        }
        break;
    }
}

int InternetConfig::
addInterface(INetworkInterface* networkInterface)
{
    int scopeID = Socket::addInterface(networkInterface);
    if (0 < scopeID && Socket::interface)
    {
        char name[3];
        sprintf(name, "%d", scopeID);
        esReport("addInterface: %s\n", name);
        Handle<IContext> folder(Socket::interface->createSubcontext(name));
        Handle<IBinding>(folder->bind("interface", networkInterface));
    }
    return scopeID;
}

IInterface* InternetConfig::
getInterface(int scopeID)
{
    Interface* interface = Socket::getInterface(scopeID);
    return interface->getNetworkInterface();
}

int InternetConfig::
getScopeID(INetworkInterface* networkInterface)
{
    return Socket::getScopeID(networkInterface);
}

void InternetConfig::
removeInterface(INetworkInterface* networkInterface)
{
    Socket::removeInterface(networkInterface);
}

void InternetConfig::
addNameServer(IInternetAddress* address)
{
    nameServers.addAddress(address);
}

IInternetAddress* InternetConfig::
getNameServer()
{
    return nameServers.getAddress();
}

void InternetConfig::
removeNameServer(IInternetAddress* address)
{
    nameServers.removeAddress(address);
}

void InternetConfig::
addSearchDomain(const char* address)
{
    // XXX sanity check name?
    char* ptr = new char[DNSHdr::NameMax];
    strcpy(ptr,address);
    domains.addLast(ptr);
    return;
}

int InternetConfig::
getSearchDomain(char* address, int addressLength)
{
    return getSearchDomain(address,addressLength, 0);
}

int InternetConfig::
getSearchDomain(char* address, int addressLength, int pos)
{
    Collection<char*>::Iterator domainIter = domains.begin();
    char* ptr;

    for (int i=0; i<=pos; i++, (ptr=domainIter.next()))
    {
        if (!domainIter.hasNext())
        {
            return 0;
        }
    }

    if (addressLength < strlen(ptr))
    {
        return 0;
    }

    strcpy(address,ptr);
    return 1;
}

void InternetConfig::
removeSearchDomain(const char* address)
{
    Collection<char*>::Iterator domainIter = domains.begin();
    char* ptr;

    while (domainIter.hasNext())
    {
        if (!strcmp(domainIter.next(), address))
        {
            ptr = domainIter.remove();
            delete ptr;
        }
    }
}

void* InternetConfig::
queryInterface(const char* riid)
{
    void* objectPtr;
    if (strcmp(riid, IInternetConfig::iid()) == 0)
    {
        objectPtr = static_cast<IInternetConfig*>(this);
    }
    else if (strcmp(riid, IInterface::iid()) == 0)
    {
        objectPtr = static_cast<IInternetConfig*>(this);
    }
    else
    {
        return 0;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
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
        char* ptr;
        while (!domains.isEmpty())
        {
            ptr = domains.removeFirst();
            delete ptr;
        }
        delete this;
        return 0;
    }
    return count;
}
