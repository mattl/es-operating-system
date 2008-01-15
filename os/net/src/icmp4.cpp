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

#include <algorithm>
#include <es/handle.h>
#include "icmp4.h"
#include "inet4address.h"

//
// ICMPReceiver
//

s16 ICMPReceiver::checksum(InetMessenger* m)
{
    int len = m->getLength();
    ICMPHdr* icmphdr = static_cast<ICMPHdr*>(m->fix(len));
    s32 sum = m->sumUp(len);
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return ~sum;
}

bool ICMPReceiver::input(InetMessenger* m, Conduit* c)
{
    int len = m->getLength();
    if (len < sizeof(ICMPHdr) || checksum(m) != 0)
    {
        return false;
    }
    return true;
}

bool ICMPReceiver::output(InetMessenger* m, Conduit* c)
{
    ICMPHdr* icmphdr = static_cast<ICMPHdr*>(m->fix(sizeof(ICMPHdr)));
    icmphdr->sum = 0;
    icmphdr->sum = checksum(m);
    m->setType(IPPROTO_ICMP);
    return true;
}

//
// ICMPEchoRequestReceiver
//

bool ICMPEchoRequestReceiver::input(InetMessenger* m, Conduit* c)
{
    esReport("ICMPEchoRequestReceiver::input\n");

    int len = m->getLength();
    int pos = 14 + 60;      // XXX Assume MAC, IPv4
    Handle<InetMessenger> r = new InetMessenger(&InetReceiver::output, pos + len, pos);

    ICMPEcho* icmphdr = reinterpret_cast<ICMPEcho*>(r->fix(len));
    memmove(icmphdr, m->fix(len), len);
    icmphdr->type = ICMPHdr::EchoReply;

    r->setRemote(m->getRemote());
    r->setLocal(m->getLocal());

    Visitor v(r);
    c->accept(&v);

    return true;
}

//
// ICMPEchoReplyReceiver
//

bool ICMPEchoReplyReceiver::input(InetMessenger* m, Conduit* c)
{
    // Resume Inet4Address::isReachable()
    notify();

    return true;
}

//
// ICMPUnreachReceiver
//

bool ICMPUnreachReceiver::input(InetMessenger* m, Conduit* c)
{
    esReport("ICMPUnreachReceiver::input\n");

    int len = m->getLength();
    if (len < sizeof(ICMPUnreach))
    {
        return false;
    }
    ICMPUnreach* icmphdr = static_cast<ICMPUnreach*>(m->fix(sizeof(ICMPUnreach)));

    Handle<Inet4Address> addr;
    switch (icmphdr->type)
    {
    case ICMPUnreach::NeedFragment:
        // Any packetization layer instance (for example, a TCP
        // connection) that is actively using the path must be notified if the
        // PMTU estimate is decreased. [RFC 1191]
        addr = m->getRemote();
        addr->setPathMTU(std::min(icmphdr->getMTU(), addr->getPathMTU()));
        m->setErrorCode(ENETUNREACH);
        break;

    // Soft error condition
    //
    // A Destination Unreachable message that is received with code
    // 0 (Net), 1 (Host), or 5 (Bad Source Route) may result from a
    // routing transient and MUST therefore be interpreted as only
    // a hint, not proof, that the specified destination is
    // unreachable [IP:11].  For example, it MUST NOT be used as
    // proof of a dead gateway (see Section 3.3.1). [RFC 1122]
    case ICMPUnreach::Net:
    case ICMPUnreach::Host:
    case ICMPUnreach::SrcFail:
    case ICMPUnreach::NetUnknown:
    case ICMPUnreach::HostUnknown:
    case ICMPUnreach::Isolated:
    case ICMPUnreach::NetProhibited:
    case ICMPUnreach::HostProhibited:
    case ICMPUnreach::NetTOS:
    case ICMPUnreach::HostTOS:
    case ICMPUnreach::Prohibited:
        // Since these Unreachable messages indicate soft error
        // conditions, TCP MUST NOT abort the connection, and it
        // SHOULD make the information available to the
        // application.
        m->setErrorCode(ENETUNREACH);
        break;

    // Hard error condition
    case ICMPUnreach::Protocol:
    case ICMPUnreach::Port:
        // These are hard error conditions, so TCP SHOULD abort
        // the connection. [RFC 1122|
        m->setErrorCode(ECONNREFUSED);
        break;
    }

    m->movePosition(sizeof(ICMPUnreach));
    m->setCommand(&InetReceiver::error);

    return true;
}

bool ICMPUnreachReceiver::output(InetMessenger* m, Conduit* c)
{
    esReport("ICMPUnreachReceiver::output\n");

    m->movePosition(-sizeof(ICMPUnreach));
    ICMPUnreach* icmphdr = static_cast<ICMPUnreach*>(m->fix(sizeof(ICMPUnreach)));
    icmphdr->type = ICMPHdr::Unreach;
    icmphdr->code = ICMPUnreach::Port;
    icmphdr->sum = 0;
    icmphdr->unused = 0;
    icmphdr->mtu = 0;

    return true;
}

//
// ICMPSourceQuenchReceiver
//

// TCP MUST react to a Source Quench by slowing transmission on the connection.
// The RECOMMENDED procedure is for a Source Quench to trigger a "slow
// start," as if a retransmission timeout had occurred. [RFC 1122]
bool ICMPSourceQuenchReceiver::input(InetMessenger* m, Conduit* c)
{
    esReport("ICMPSourceQuenchReceiver::input\n");

    int len = m->getLength();
    if (len < sizeof(ICMPSourceQuench))
    {
        return false;
    }
    ICMPSourceQuench* icmphdr = static_cast<ICMPSourceQuench*>(m->fix(sizeof(ICMPSourceQuench)));

    m->movePosition(sizeof(ICMPSourceQuench));
    m->setCommand(&InetReceiver::error);

    return true;
}

bool ICMPSourceQuenchReceiver::output(InetMessenger* m, Conduit* c)
{
    esReport("ICMPSourceQuenchReceiver::output\n");

    m->movePosition(-sizeof(ICMPSourceQuench));
    ICMPSourceQuench* icmphdr = static_cast<ICMPSourceQuench*>(m->fix(sizeof(ICMPSourceQuench)));
    icmphdr->type = ICMPHdr::SourceQuench;
    icmphdr->code = m->getType();
    icmphdr->sum = 0;
    icmphdr->unused = 0;

    return true;
}

//
// ICMPTimeExceededReceiver
//

// This should be handled the same way as Destination
// Unreachable codes 0, 1, 5 [RFC 1122]
bool ICMPTimeExceededReceiver::input(InetMessenger* m, Conduit* c)
{
    esReport("ICMPTimeExceededReceiver::input\n");

    int len = m->getLength();
    if (len < sizeof(ICMPTimeExceeded))
    {
        return false;
    }
    ICMPTimeExceeded* icmphdr = static_cast<ICMPTimeExceeded*>(m->fix(sizeof(ICMPTimeExceeded)));

    m->setErrorCode(ENETUNREACH);
    m->movePosition(sizeof(ICMPTimeExceeded));
    m->setCommand(&InetReceiver::error);

    return true;
}

bool ICMPTimeExceededReceiver::output(InetMessenger* m, Conduit* c)
{
    esReport("ICMPTimeExceededReceiver::output\n");

    m->movePosition(-sizeof(ICMPTimeExceeded));
    ICMPTimeExceeded* icmphdr = static_cast<ICMPTimeExceeded*>(m->fix(sizeof(ICMPTimeExceeded)));
    icmphdr->type = ICMPHdr::TimeExceeded;
    icmphdr->code = m->getType();
    icmphdr->sum = 0;
    icmphdr->unused = 0;

    return true;
}

//
// ICMPParamProbReceiver
//

// This should be handled the same way as Destination
// Unreachable codes 0, 1, 5 [RFC 1122]
bool ICMPParamProbReceiver::input(InetMessenger* m, Conduit* c)
{
    esReport("ICMPParamProbReceiver::input\n");

    int len = m->getLength();
    if (len < sizeof(ICMPParamProb))
    {
        return false;
    }
    ICMPParamProb* icmphdr = static_cast<ICMPParamProb*>(m->fix(sizeof(ICMPParamProb)));

    m->setErrorCode(ENETUNREACH);
    m->movePosition(sizeof(ICMPParamProb));
    m->setCommand(&InetReceiver::error);

    return true;
}

bool ICMPParamProbReceiver::output(InetMessenger* m, Conduit* c)
{
    esReport("ICMPParamProbReceiver::output\n");

    m->movePosition(-sizeof(ICMPParamProb));
    ICMPParamProb* icmphdr = static_cast<ICMPParamProb*>(m->fix(sizeof(ICMPParamProb)));
    icmphdr->type = ICMPHdr::ParamProb;
    icmphdr->code = m->getType();
    icmphdr->sum = 0;
    icmphdr->ptr = 0;   // XXX
    icmphdr->unused0 = icmphdr->unused1 = icmphdr->unused2 = 0;

    return true;
}
