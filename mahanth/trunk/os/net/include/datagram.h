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

#ifndef DATAGRAM_H_INCLUDED
#define DATAGRAM_H_INCLUDED

#include <es/endian.h>
#include <es/ring.h>
#include <es/synchronized.h>
#include <es/base/IMonitor.h>
#include "inet.h"
#include "socket.h"

class DatagramReceiver :
    public SocketReceiver
{
    es::Monitor*   monitor;
    u8*         recvBuf;
    Ring        recvRing;
    u8*         sendBuf;
    Ring        sendRing;
    Conduit*    conduit;
    int         errorCode;
    Socket*     socket;

    struct RingHdr
    {
        long        len;
        Address*    addr;
        int         port;

        RingHdr(long len = 0, Address* addr = 0, int port = 0) :
            len(len),
            addr(addr),
            port(port)
        {
        }
    };

    bool isAcceptable()
    {
        return false;
    }
    bool isConnectable()
    {
        return true;
    }
    bool isReadable()
    {
        return 0 < 0 < recvRing.getUsed() || errorCode;
    }
    bool isWritable()
    {
        return true;
    }

public:
    DatagramReceiver(Conduit* conduit = 0) :
        monitor(0),
        recvBuf(0),
        sendBuf(0),
        conduit(conduit),
        errorCode(0),
        socket(0)
    {
        monitor = es::Monitor::createInstance();
    }

    ~DatagramReceiver()
    {
        if (recvBuf)
        {
            delete[] recvBuf;
        }
        if (sendBuf)
        {
            delete[] sendBuf;
        }
        if (monitor)
        {
            monitor->release();
        }
    }

    bool initialize(Socket* socket)
    {
        this->socket = socket;
        recvBuf = new u8[socket->getReceiveBufferSize()];
        recvRing.initialize(recvBuf, socket->getReceiveBufferSize());
        sendBuf = new u8[socket->getSendBufferSize()];
        sendRing.initialize(sendBuf, socket->getSendBufferSize());
        return true;
    }

    void notify()
    {
        monitor->notifyAll();
        if (socket->selector)
        {
            socket->selector->notifyAll();
        }
    }

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
    bool error(InetMessenger* m, Conduit* c);

    bool read(SocketMessenger* m, Conduit* c);
    bool write(SocketMessenger* m, Conduit* c);
    bool close(SocketMessenger* m, Conduit* c);
    bool notify(SocketMessenger* m, Conduit* c);

    bool isAcceptable(SocketMessenger* m, Conduit* c)
    {
        m->setErrorCode(isAcceptable());
        return false;
    }

    bool isConnectable(SocketMessenger* m, Conduit* c)
    {
        m->setErrorCode(isConnectable());
        return false;
    }

    bool isReadable(SocketMessenger* m, Conduit* c)
    {
        m->setErrorCode(isReadable());
        return false;
    }

    bool isWritable(SocketMessenger* m, Conduit* c)
    {
        m->setErrorCode(isWritable());
        return false;
    }

    DatagramReceiver* clone(Conduit* conduit, void* key)
    {
        return new DatagramReceiver(conduit);
    }

    unsigned int release()
    {
        delete this;
        return 0;
    }
};

#endif  // DATAGRAM_H_INCLUDED
