/*
 * Copyright 2008 Google Inc.
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

#include <es/handle.h>
#include <es/net/inet4.h>
#include "tcp.h"

s16 TCPReceiver::
checksum(InetMessenger* m)
{
    int len = m->getLength();
    s32 sum = m->sumUp(len);
    Handle<Address> addr;
    addr = m->getRemote();
    sum += addr->sumUp();
    addr = m->getLocal();
    sum += addr->sumUp();
    sum += htons(len);
    sum += ntohs(IPPROTO_TCP);
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return ~sum;
}

bool TCPReceiver::input(InetMessenger* m, Conduit* c)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    if (!tcphdr)
    {
        return false;
    }
    int hlen = tcphdr->getHdrSize();
    if (hlen < sizeof(TCPHdr) || m->getLength() < hlen)
    {
        return false;
    }

    // Drop broadcast or multicast
    Handle<Address> src = m->getRemote();
    Handle<Address> dst = m->getLocal();
    if (src->isUnspecified() || src->isMulticast() ||
        dst->isUnspecified() || dst->isMulticast()) // XXX check for bcast
    {
        return false;
    }

    // Verify the sum
    if (checksum(m) != 0)
    {
        return false;
    }

    m->setRemotePort(ntohs(tcphdr->src));
    m->setLocalPort(ntohs(tcphdr->dst));

    return true;
}

bool TCPReceiver::output(InetMessenger* m, Conduit* c)
{
    // Update TCPHdr
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));
    tcphdr->src = htons(m->getLocalPort());
    tcphdr->dst = htons(m->getRemotePort());
    tcphdr->sum = 0;
    tcphdr->sum = checksum(m);
    m->setType(IPPROTO_TCP);

    return true;
}

bool TCPReceiver::error(InetMessenger* m, Conduit* c)
{
    TCPHdr* tcphdr = static_cast<TCPHdr*>(m->fix(sizeof(TCPHdr)));

    // Reverse src and dst
    m->setRemotePort(ntohs(tcphdr->dst));
    m->setLocalPort(ntohs(tcphdr->src));

    return true;
}
