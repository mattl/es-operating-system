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

#ifndef INET4REASS_H_INCLUDED
#define INET4REASS_H_INCLUDED

#include "inet.h"

class ReassReceiver :
    public InetReceiver,
    public InetMessenger,
    public TimerTask
{
    static const u16 EOH = 1;   // end-of-hole mark (EOH % sizeof(Hole) != 0)

    struct Hole
    {
        u16 first;
        u16 last;
        u16 next;
        u16 _unused;
    };

    // The first fragment header needs to be saved for inclusion in a
    // possible ICMP Time Exceeded (Reassembly Timeout) message. [RFC 1122]
    u8              firstFragment[IPHdr::MaxHdrSize + 8];

    Protocol*       inProtocol;
    Protocol*       timeExceededProtocol;
    InetMessenger*  r;
    u16             list;

    Conduit*        adapter;

public:
    ReassReceiver(Protocol* inProtocol, Protocol* timeExceededProtocol, Conduit* adapter) :
        inProtocol(inProtocol),
        timeExceededProtocol(timeExceededProtocol),
        r(0),
        adapter(adapter)
    {
    }

    bool input(InetMessenger* m, Conduit* c);

    ReassReceiver* clone(Conduit* conduit, void* key)
    {
        return new ReassReceiver(inProtocol, timeExceededProtocol, conduit);
    }

    unsigned int release()
    {
        delete this;
        return 0;
    }

    void run();
};

class ReassFactoryReceiver : public InetReceiver
{
public:
    bool input(InetMessenger* m, Conduit* c);
};

#endif  // INET4REASS_H_INCLUDED
