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

#include <string.h>
#include <new>
#include <es/handle.h>
#include <es/net/dix.h>
#include "inet4.h"

ARPFamily::ARPFamily(InFamily* inFamily) :
    inFamily(inFamily),
    scopeMux(&scopeAccessor, &scopeFactory),
    arpReceiver(inFamily),
    arpFactory(&arpAdapter),
    arpMux(&arpAccessor, &arpFactory)
{
    ASSERT(inFamily);
    setCacheSize(3); //XXX: Default Cache Size

    arpProtocol.setReceiver(&arpReceiver);
    arpAdapter.setReceiver(&inFamily->addressAny);

    Conduit::connectAA(&scopeMux, &arpProtocol);
    Conduit::connectBA(&arpProtocol, &arpMux);

    Socket::addAddressFamily(this);
}

// Get Least Recently Used Address from Cache

Inet4Address* ARPFamily::getLRUAddress()
{
    u32 numEntries = 0;
    Inet4Address* LRUaddr = 0;
    Tree<void*, Conduit*>::Node* node;
    Tree<void*, Conduit*>::Iterator iter = arpMux.list();
    while ((node = iter.next()))
    {
        Conduit* conduit = node->getValue();
        Inet4Address* addr = dynamic_cast<Inet4Address*>(conduit->getReceiver());
        ASSERT(addr);
        if (!LRUaddr || (LRUaddr->getLastUsed() > addr->getLastUsed()))
        {
            if (!addr->isPreferred() && addr != &inFamily->addressAny)
            {
                LRUaddr = addr;
            }
        }
        numEntries++;
    }
    if (numEntries < cacheSize)
    {
        // Cache not full
        return 0;
    } 
    return LRUaddr;
}

// StateInit

void Inet4Address::
StateInit::start(Inet4Address* a)
{
    // Install ARP cache for this address.
    a->inFamily->arpFamily.addAddress(a);

    a->setState(stateIncomplete);
    a->run();   // Invoke expired
}

bool Inet4Address::
StateInit::input(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateInit::output(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateInit::error(InetMessenger* m, Inet4Address* a)
{
    return true;
}

// StateIncomplete

void Inet4Address::
StateIncomplete::expired(Inet4Address* a)
{
    // Send request
    if (++a->timeoutCount <= 6)
    {
        esReport("StateIncomplete::timeoutCount: %d\n", a->timeoutCount);

        Handle<Inet4Address> src = a->inFamily->selectSourceAddress(a);
        if (!src)
        {
            a->setState(stateInit);
            return;
        }

        int len = sizeof(DIXHdr) + sizeof(ARPHdr);
        Handle<InetMessenger> m = new InetMessenger(&InetReceiver::output, len, sizeof(DIXHdr));
        m->setScopeID(a->getScopeID());

        ARPHdr* arphdr = static_cast<ARPHdr*>(m->fix(sizeof(ARPHdr)));
        memset(arphdr, 0, sizeof(ARPHdr));
        arphdr->tpa = a->getAddress();

        arphdr->spa = src->getAddress();
        src->getMacAddress(arphdr->sha);

        arphdr->op = htons(ARPHdr::OP_REQUEST);

        Visitor v(m);
        a->adapter->accept(&v);
        a->alarm(5000000LL << a->timeoutCount);
    }
    else
    {
        // Not found:
        a->setState(stateInit);
    }
}

bool Inet4Address::
StateIncomplete::input(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateIncomplete::output(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateIncomplete::error(InetMessenger* m, Inet4Address* a)
{
    return true;
}

// StateReachable

void Inet4Address::
StateReachable::start(Inet4Address* a)
{
    a->alarm(20 * 60 * 10000000LL);

    // Send kept packets
    while (Handle<InetMessenger> m = a->retrieve())
    {
        Visitor v(m);
        Conduit* conduit = &a->inFamily->scopeMux;
        conduit->accept(&v, conduit->getA());
    }
}

void Inet4Address::
StateReachable::expired(Inet4Address* a)
{
    a->setState(stateProbe);
}

bool Inet4Address::
StateReachable::input(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateReachable::output(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateReachable::error(InetMessenger* m, Inet4Address* a)
{
    return true;
}

// StateProbe

void Inet4Address::
StateProbe::expired(Inet4Address* a)
{
    // Send probe
    if (++a->timeoutCount <= 6)
    {
        esReport("StateProbe::timeoutCount: %d\n", a->timeoutCount);

        Handle<Inet4Address> src = a->inFamily->selectSourceAddress(a);
        if (!src)
        {
            a->setState(stateInit);
            return;
        }

        int len = sizeof(DIXHdr) + sizeof(ARPHdr);
        Handle<InetMessenger> m = new InetMessenger(&InetReceiver::output, len, sizeof(DIXHdr));
        m->setScopeID(a->getScopeID());

        ARPHdr* arphdr = static_cast<ARPHdr*>(m->fix(sizeof(ARPHdr)));
        memset(arphdr, 0, sizeof(ARPHdr));
        arphdr->tpa = a->getAddress();
        a->getMacAddress(arphdr->tha);  // Send directly to tha

        arphdr->spa = src->getAddress();
        src->getMacAddress(arphdr->sha);

        arphdr->op = htons(ARPHdr::OP_REQUEST);

        Visitor v(m);
        a->adapter->accept(&v);
        a->alarm(5000000LL << a->timeoutCount);
    }
    else
    {
        // Not found:
        a->setState(stateInit);
        // Uninstall ARP cache for this address.
        a->inFamily->arpFamily.removeAddress(a);
    }
}

bool Inet4Address::
StateProbe::input(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateProbe::output(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateProbe::error(InetMessenger* m, Inet4Address* a)
{
    return true;
}

// StateTentative

void Inet4Address::
StateTentative::start(Inet4Address* a)
{
    // Set the interface MAC address to this address.
    NetworkInterface* nic = Socket::getInterface(a->getScopeID());
    ASSERT(nic);
    u8 mac[6];
    nic->getMacAddress(mac);
    a->setMacAddress(mac);

    // Install ARP cache for this address.
    a->inFamily->arpFamily.addAddress(a);

    a->alarm(rand48() % (ARPHdr::PROBE_WAIT * 10000000LL));
}

void Inet4Address::
StateTentative::expired(Inet4Address* a)
{
    ASSERT(a->adapter);
    if (++a->timeoutCount <= ARPHdr::PROBE_NUM)
    {
        esReport("StateTentative::timeoutCount: %d\n", a->timeoutCount);

        // Send probe
        int len = sizeof(DIXHdr) + sizeof(ARPHdr);
        Handle<InetMessenger> m = new InetMessenger(&InetReceiver::output, len, sizeof(DIXHdr));
        m->setScopeID(a->getScopeID());

        ARPHdr* arphdr = static_cast<ARPHdr*>(m->fix(sizeof(ARPHdr)));
        memset(arphdr, 0, sizeof(ARPHdr));
        arphdr->tpa = a->getAddress();
        arphdr->spa = InAddrAny;
        a->getMacAddress(arphdr->sha);
        arphdr->op = htons(ARPHdr::OP_REQUEST);

        Visitor v(m);
        a->adapter->accept(&v);
        if (a->timeoutCount < ARPHdr::PROBE_NUM)
        {
            a->alarm(ARPHdr::PROBE_MIN * 10000000LL + rand48() % (ARPHdr::PROBE_MAX * 10000000LL));
        }
        else
        {
            a->alarm(ARPHdr::ANNOUNCE_WAIT * 10000000LL);
        }
    }
    else
    {
        a->setState(statePreferred);
        a->start();
        a->run();
    }
}

bool Inet4Address::
StateTentative::input(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateTentative::output(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateTentative::error(InetMessenger* m, Inet4Address* a)
{
    return true;
}

// StatePreferred

void Inet4Address::
StatePreferred::start(Inet4Address* a)
{
    // Install an ICMP echo request adapter for this address.
    InetMessenger m;
    m.setLocal(a);
    Installer installer(&m);
    a->inFamily->echoRequestMux.accept(&installer, &a->inFamily->icmpMux);
}

void Inet4Address::
StatePreferred::expired(Inet4Address* a)
{
    ASSERT(a->adapter);
    if (++a->timeoutCount <= ARPHdr::ANNOUNCE_NUM)
    {
        esReport("StatePreferred::timeoutCount: %d\n", a->timeoutCount);

        // Send announcement
        int len = sizeof(DIXHdr) + sizeof(ARPHdr);
        Handle<InetMessenger> m = new InetMessenger(&InetReceiver::output, len, sizeof(DIXHdr));
        m->setScopeID(a->getScopeID());

        ARPHdr* arphdr = static_cast<ARPHdr*>(m->fix(sizeof(ARPHdr)));
        memset(arphdr, 0, sizeof(ARPHdr));
        arphdr->tpa = a->getAddress();
        a->getMacAddress(arphdr->tha);
        arphdr->spa = a->getAddress();;
        a->getMacAddress(arphdr->sha);
        arphdr->op = htons(ARPHdr::OP_REQUEST);

        Visitor v(m);
        a->adapter->accept(&v);
        a->alarm(ARPHdr::ANNOUNCE_INTERVAL * 10000000LL);
    }
}

bool Inet4Address::
StatePreferred::input(InetMessenger* m, Inet4Address* a)
{
    ARPHdr* arphdr = static_cast<ARPHdr*>(m->fix(sizeof(ARPHdr)));
    if (ntohs(arphdr->op) == ARPHdr::OP_REQUEST)
    {
        esReport("StatePreferred::input: %02x:%02x:%02x:%02x:%02x:%02x\n",
                 arphdr->tha[0], arphdr->tha[1], arphdr->tha[2],
                 arphdr->tha[3], arphdr->tha[4], arphdr->tha[5]);

        // Send reply
        int len = sizeof(DIXHdr) + sizeof(ARPHdr);
        Handle<InetMessenger> r = new InetMessenger(&InetReceiver::output, len, sizeof(DIXHdr));
        r->setScopeID(a->getScopeID());

        ARPHdr* rephdr = static_cast<ARPHdr*>(r->fix(sizeof(ARPHdr)));
        memset(rephdr, 0, sizeof(ARPHdr));

        rephdr->tpa = arphdr->spa;
        memmove(rephdr->tha, arphdr->sha, 6);

        rephdr->spa = a->getAddress();;
        a->getMacAddress(rephdr->sha);
        rephdr->op = htons(ARPHdr::OP_REPLY);

        Visitor v(r);
        a->adapter->accept(&v);
    }
    return true;
}

bool Inet4Address::
StatePreferred::output(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StatePreferred::error(InetMessenger* m, Inet4Address* a)
{
    return true;
}

// StateDeprecated

void Inet4Address::
StateDeprecated::start(Inet4Address* a)
{
    // Uninstall ARP cache for this address.
    a->inFamily->arpFamily.removeAddress(a);

    // Install an ICMP echo request adapter for this address if necessary.
    InetMessenger m;
    m.setLocal(a);
    void* key = a->inFamily->echoRequestMux.getKey(&m);
    Adapter* adapter = dynamic_cast<Adapter*>(a->inFamily->echoRequestMux.getB(key));
    if (adapter)
    {
        Uninstaller uninstaller(&m);
        adapter->accept(&uninstaller);
    }
}

bool Inet4Address::
StateDeprecated::input(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateDeprecated::output(InetMessenger* m, Inet4Address* a)
{
    return true;
}

bool Inet4Address::
StateDeprecated::error(InetMessenger* m, Inet4Address* a)
{
    return true;
}

// ARPReceiver

bool ARPReceiver::
input(InetMessenger* m, Conduit* c)
{
    ASSERT(m);

    // Validates the received ARP packet.
    ARPHdr* arphdr = static_cast<ARPHdr*>(m->fix(sizeof(ARPHdr)));
    if (ntohs(arphdr->hrd) != ARPHdr::HRD_ETHERNET ||
        ntohs(arphdr->pro) != ARPHdr::PRO_IP ||
        arphdr->hln != 6 ||
        arphdr->pln != 4)
    {
        return false;
    }

    m->setType(AF_ARP);

    Handle<Inet4Address> addr;

    addr = inFamily->getAddress(arphdr->spa, m->getScopeID());
    if (addr)
    {
        addr->setMacAddress(arphdr->sha);
        addr->cancel();
        if (addr->isLocalAddress())
        {
            addr->setState(Inet4Address::stateDeprecated);
        }
        else
        {
            addr->setState(Inet4Address::stateReachable);
        }
        addr->start();  // To send waiting packets.
    }

    addr = inFamily->getAddress(arphdr->tpa, m->getScopeID());
    m->setLocal(addr);

    return true;
}
