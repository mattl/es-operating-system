/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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
#include "visualizer.h"

bool ReassReceiver::input(InetMessenger* m, Conduit* c)
{
    esReport("ReassReceiver::input\n");

    m->restorePosition();   // Back to IPHdr
    IPHdr* frag = static_cast<IPHdr*>(m->fix(sizeof(IPHdr)));

    if (!r)
    {
        list = 0;
        firstFragment[0] = 0;
        setRemote(Handle<Address>(m->getRemote()));
        setLocal(Handle<Address>(m->getLocal()));
        setRemotePort(m->getRemotePort());

        r = new InetMessenger(&InetReceiver::input, 65535); // XXX
        Socket::alarm(this, 600000000LL);

        memmove(r->fix(sizeof(IPHdr)), m->fix(sizeof(IPHdr)), sizeof(IPHdr));
        Hole* hole = (Hole*) r->fix(sizeof(Hole), sizeof(IPHdr));
        hole->first = 0;
        hole->last  = 65535 - sizeof(IPHdr);
        hole->next  = EOH;

        Visualizer v;
        inProtocol->accept(&v);
    }
    IPHdr* iphdr = static_cast<IPHdr*>(r->fix(sizeof(IPHdr)));

    int first = frag->getOffset();
    int len = frag->getSize() - frag->getHdrSize();
    int last = first + len - 1;
    if (len <= 0 || (frag->moreFragments() && len % 8 != 0))
    {
        r->release();
        r = 0;
        Socket::cancel(this);

        // Uninstall this conduits
        Uninstaller uninstaller(m);
        adapter->accept(&uninstaller);

        return true;
    }

    int offset = frag->getOffset();
    if (offset == 0 && frag->getHdrSize() + 8 <= frag->getSize() && !firstFragment[0])
    {
        memmove(firstFragment, frag, frag->getHdrSize() + 8);
        // XXX Check the size of datagram
        if (sizeof(IPHdr) < frag->getHdrSize())
        {
            memmove((char*) iphdr + frag->getHdrSize(),
                    (char*) iphdr + sizeof(IPHdr),
                    iphdr->getSize() - sizeof(IPHdr));
            memmove(iphdr, frag, frag->getHdrSize());
        }
    }

    // Update iphdr
    if (iphdr->getSize() < iphdr->getHdrSize() + first + len)
    {
        iphdr->setSize(iphdr->getHdrSize() + first + len);
    }

    u16* next = &list;
    while (*next != EOH)
    {
        Hole* hole = (Hole*) ((u8*) iphdr + iphdr->getHdrSize() + *next);
        if (hole->last < first || last < hole->first)
        {
            next = &hole->next;
            continue;
        }

        u16 holeFirst = hole->first;
        u16 holeLast = hole->last;
        *next = hole->next; // Delete current hole

        if (holeFirst < first)
        {
            Hole* newHole = hole;
            newHole->last = (u16) (first - 1);

            *next = newHole->first;
            next = &newHole->next;

            offset = 0;
        }
        else // if (first <= holeFirst)
        {
            offset = holeFirst - first;
        }

        if (last < holeLast)
        {
            if (frag->moreFragments())
            {
                Hole* newHole = (Hole*) ((u8*) iphdr + iphdr->getHdrSize() + last + 1);
                newHole->first = (u16) (last + 1);
                newHole->last = holeLast;
                newHole->next = *next;

                *next = newHole->first;
                next = &newHole->next;
            }
            else
            {
                *next = EOH;
            }
        }
        else // if (holeLast <= last)
        {
            last = holeLast;
        }

        ASSERT(0 <= last - (first + offset) + 1);   // Teardrop check
        memmove((u8*) iphdr + iphdr->getHdrSize() + first + offset,
                (u8*) frag + frag->getHdrSize() + offset,
                (u32) (last - (first + offset) + 1));
    }

    if (list == EOH)
    {
        Socket::cancel(this);

        // Send r
        r->setSize(iphdr->getSize());
        r->setLocal(Handle<Address>(getLocal()));
        r->setRemote(Handle<Address>(getRemote()));
        r->setType(iphdr->proto);
        r->savePosition();
        r->movePosition(iphdr->getHdrSize());
        Transporter v(r);
        inProtocol->getB()->accept(&v, inProtocol);

        r->release();
        r = 0;

        // Uninstall this conduits
        Uninstaller uninstaller(m);
        adapter->accept(&uninstaller);
    }

    return true;
}

// Running out of time.
void ReassReceiver::run()
{
    IPHdr* header = reinterpret_cast<IPHdr*>(firstFragment);

    // Time-to-live check (60 sec)
    if (r)
    {
        // Send ICMP time exceeded on reassembly timeout [RFC 1122 MUST]
        IPHdr* first = reinterpret_cast<IPHdr*>(firstFragment);
        if (first->getVersion() == 4)   // Is the first fragment saved?
        {
            int len = first->getHdrSize() + 8;
            int pos = 14 + 60 + sizeof(ICMPTimeExceeded);    // XXX Assume MAC, IPv4
            Handle<InetMessenger> e = new InetMessenger(&InetReceiver::output, pos + len, pos);

            memmove(e->fix(len), first, len);
            e->setRemote(Handle<Address>(getRemote()));
            e->setLocal(Handle<Address>(getLocal()));
            e->setType(ICMPTimeExceeded::FragmentTimeout);
            Visitor v(e);
            timeExceededProtocol->accept(&v, timeExceededProtocol->getB());
        }

        r->release();
        r = 0;
    }

    // Uninstall this conduits
    Uninstaller uninstaller(this);
    adapter->accept(&uninstaller);
}

// c is a pointer to a conduit factory.
bool ReassFactoryReceiver::input(InetMessenger* m, Conduit* c)
{
    Installer installer(m);
    Conduit* mux = c->getA();
    mux->accept(&installer, mux->getA());
    return false;
}
