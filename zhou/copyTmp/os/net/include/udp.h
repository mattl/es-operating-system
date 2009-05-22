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

#ifndef UDP_H_INCLUDED
#define UDP_H_INCLUDED

#include <es/endian.h>
#include <es/net/udp.h>
#include "inet.h"

// IPv4 UDP Receiver
class UDPReceiver : public InetReceiver
{
    s16 checksum(InetMessenger* m);

public:
    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
    bool error(InetMessenger* m, Conduit* c);
};

class UDPUnreachReceiver : public InetReceiver
{
    Protocol*   unreachProtocol;

public:
    UDPUnreachReceiver(Protocol* unreachProtocol) :
        unreachProtocol(unreachProtocol)
    {
    }

    bool input(InetMessenger* m, Conduit* c);
};

#endif  // UDP_H_INCLUDED
