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

#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

#include <errno.h>
#include <es/collection.h>
#include <es/endian.h>
#include <es/ref.h>
#include <es/timer.h>
#include <es/base/ISelectable.h>
#include <es/base/IStream.h>
#include <es/naming/IContext.h>
#include <es/net/IInternetConfig.h>
#include <es/net/IResolver.h>
#include <es/net/ISocket.h>
#include <es/net/udp.h>
#include "inet.h"
#include "interface.h"
#include "address.h"

class AddressFamily;
class Socket;

class AddressFamily
{
    Link<AddressFamily> link;

public:
    virtual int getAddressFamily() = 0;
    virtual Conduit* getProtocol(Socket* socket) = 0;
    virtual int selectEphemeralPort(Socket* socket)
    {
        return 0;
    };
    virtual void addInterface(Interface* interface) = 0;
    virtual IInternetAddress* selectSourceAddress(IInternetAddress* dst)
    {
        return 0;
    }

    friend class Socket;
    typedef ::List<AddressFamily, &AddressFamily::link> List;
};

class Socket :
    public IMulticastSocket,
    public ISelectable,
    public InetReceiver,
    public InetMessenger
{
public:
    static const int INTERFACE_MAX = 8;

    static IResolver*       resolver;
    static IInternetConfig* config;
    static IContext*        interface;

private:
    static AddressFamily::List  addressFamilyList;
    static Interface*           interfaces[INTERFACE_MAX];
    static Timer*               timer;

    Ref             ref;
    int             family;
    int             type;
    int             protocol;
    Adapter*        adapter;
    AddressFamily*  af;
    int             recvBufferSize;
    int             sendBufferSize;
    int             errorCode;
    TimeSpan        timeout;
    Collection<Address*>    addresses;

    // Asynchronous I/O
    IMonitor*       selector;
    bool            blocking;

public:
    static void initialize();

    static void addAddressFamily(AddressFamily* af)
    {
        addressFamilyList.addLast(af);
    }

    static void removeAddressFamily(AddressFamily* af)
    {
        addressFamilyList.remove(af);
    }

    static AddressFamily* getAddressFamily(int key)
    {
        AddressFamily* af;
        AddressFamily::List::Iterator iter = addressFamilyList.begin();
        while ((af = iter.next()))
        {
            if (af->getAddressFamily() == key)
            {
                return af;
            }
        }
        return 0;
    }

    Conduit* getProtocol()
    {
        return af->getProtocol(this);
    }

    static int addInterface(INetworkInterface* networkInterface);
    static void removeInterface(INetworkInterface* networkInterface);
    static Interface* getInterface(int scopeID)
    {
        if (scopeID < 1 || INTERFACE_MAX <= scopeID)
        {
            return 0;
        }
        return interfaces[scopeID];
    }
    static int getScopeID(INetworkInterface* networkInterface)
    {
        for (int id = 1; id < INTERFACE_MAX; ++id)
        {
            if (networkInterface == interfaces[id]->networkInterface)
            {
                return id;
            }
        }
        return 0;
    }

    static void alarm(TimerTask* timerTask, TimeSpan delay)
    {
        timer->schedule(timerTask, delay);
    }

    static void cancel(TimerTask* timerTask)
    {
        timer->cancel(timerTask);
    }


    Socket(int family, int type, int protocol = 0);
    ~Socket();

    bool input(InetMessenger* m, Conduit* c);
    bool output(InetMessenger* m, Conduit* c);
    bool error(InetMessenger* m, Conduit* c);

    Adapter* getAdapter() const
    {
        return adapter;
    }
    void setAdapter(Adapter* adapter)
    {
        this->adapter = adapter;
    }

    Receiver* getReceiver()
    {
        if (!adapter)
        {
            return 0;
        }
        Conduit* conduit = adapter->getA();
        if (!conduit)
        {
            return 0;
        }
        return conduit->getReceiver();
    }

    //
    // ISocket
    //

    int getAddressFamily()
    {
        return family;
    }
    IInternetAddress* getLocalAddress()
    {
        return getLocal();
    }
    int getLocalPort()
    {
        return InetMessenger::getLocalPort();
    }
    int getProtocolType()
    {
        return protocol;
    }
    IInternetAddress* getRemoteAddress()
    {
        return getRemote();
    }
    int getRemotePort()
    {
        return InetMessenger::getRemotePort();
    }
    int getSocketType()
    {
        return type;
    }
    int getLastError()
    {
        return errorCode;
    }

    bool isBound();
    bool isClosed();
    bool isConnected();

    int getHops();
    void setHops(int limit);

    int getReceiveBufferSize();
    void setReceiveBufferSize(int size);

    int getSendBufferSize();
    void setSendBufferSize(int size);

    bool isReuseAddress();
    void setReuseAddress(bool on);

    long long getTimeout();
    void setTimeout(long long timeSpan);

    ISocket* accept();
    void bind(IInternetAddress* addr, int port);
    void close();
    void connect(IInternetAddress* addr, int port);
    void listen(int backlog);
    int read(void* dst, int count);
    int recvFrom(void* dst, int count, int flags, IInternetAddress** addr, int* port);
    int sendTo(const void* src, int count, int flags, IInternetAddress* addr, int port);
    void shutdownInput();
    void shutdownOutput();
    int write(const void* src, int count);

    void notify();

    bool isBlocking()
    {
        return blocking;
    }
    void setBlocking(bool on)
    {
        blocking = on;
    }

    bool isAcceptable();
    bool isConnectable();
    bool isReadable();
    bool isWritable();

    // ISelectable
    int add(IMonitor* selector);
    int remove(IMonitor* selector);

    // IMulticastSocket
    bool isLoopbackMode();
    void setLoopbackMode(bool disable);
    void joinGroup(IInternetAddress* addr);
    void leaveGroup(IInternetAddress* addr);

    //
    // IInterface
    //
    void* queryInterface(const Guid& riid);
    unsigned int addRef();
    unsigned int release();

    friend class StreamReceiver;
    friend class DatagramReceiver;
};

class SocketMessenger;
class SocketReceiver : public InetReceiver
{
public:
    virtual bool initialize(Socket* socket)
    {
        return true;
    }

    virtual bool read(SocketMessenger* m, Conduit* c)
    {
        return true;
    }

    virtual bool write(SocketMessenger* m, Conduit* c)
    {
        return true;
    }

    virtual bool accept(SocketMessenger* m, Conduit* c)
    {
        return false;
    }

    virtual bool connect(SocketMessenger* m, Conduit* c)
    {
        return false;
    }

    virtual bool close(SocketMessenger* m, Conduit* c)
    {
        return false;
    }

    virtual bool shutdownOutput(SocketMessenger* m, Conduit* c)
    {
        return false;
    }

    virtual bool shutdownInput(SocketMessenger* m, Conduit* c)
    {
        return false;
    }

    virtual bool isAcceptable(SocketMessenger* m, Conduit* c)
    {
        return false;
    }

    virtual bool isConnectable(SocketMessenger* m, Conduit* c)
    {
        return false;
    }

    virtual bool isReadable(SocketMessenger* m, Conduit* c)
    {
        return false;
    }

    virtual bool isWritable(SocketMessenger* m, Conduit* c)
    {
        return false;
    }

    virtual bool notify(SocketMessenger* m, Conduit* c)
    {
        return false;
    }

    typedef bool (SocketReceiver::*Command)(SocketMessenger*, Conduit*);
};

class SocketMessenger : public InetMessenger
{
    Socket* socket;
    SocketReceiver::Command op;

public:
    SocketMessenger(Socket* socket, SocketReceiver::Command op,
                    void* chunk = 0, long len = 0, long pos = 0) :
        InetMessenger(0, len, pos, chunk),
        socket(socket),
        op(op)
    {
        if (socket)
        {
            socket->addRef();
            setLocal(socket->getLocal());
            setRemote(socket->getRemote());
            setLocalPort(socket->getLocalPort());
            setRemotePort(socket->getRemotePort());
        }
    }

    ~SocketMessenger()
    {
        if (socket)
        {
            socket->release();
        }
    }

    virtual bool apply(Conduit* c)
    {
        if (op)
        {
            if (SocketReceiver* receiver = dynamic_cast<SocketReceiver*>(c->getReceiver()))
            {
                return (receiver->*op)(this, c);
            }
        }
        return InetMessenger::apply(c);
    }

    void setCommand(InetReceiver::Command op)
    {
        InetMessenger::op = op;
        op = 0;
    }

    Socket* getSocket()
    {
        if (socket)
        {
            socket->addRef();
        }
        return socket;
    }
    void setSocket(Socket* socket)
    {
        if (socket)
        {
            socket->addRef();
        }
        if (this->socket)
        {
            this->socket->release();
        }
        this->socket = socket;
    }
};

class SocketInstaller : public Visitor
{
    Socket* socket;
    int     code;

public:
    SocketInstaller(Socket* s) :
        Visitor(s),
        socket(s),
        code(0)
    {
    }
    bool at(Protocol* p, Conduit* c)
    {
        if (p->getB())
        {
            code = EADDRINUSE;
            return true;
        }
        Adapter* adapter = new Adapter;
        socket->addRef();
        adapter->setReceiver(socket);
        socket->setAdapter(adapter);
        Conduit::connectAB(adapter, p);

        SocketReceiver* receiver = dynamic_cast<SocketReceiver*>(p->getReceiver());
        ASSERT(receiver);
        receiver->initialize(socket);

        return true;
    }
    bool at(ConduitFactory* f, Conduit* c)
    {
        Mux* mux = dynamic_cast<Mux*>(c);
        if (mux)
        {
            ASSERT(mux->getFactory() == f);
            void* key = mux->getKey(socket);
            Conduit* e = f->create(key);
            if (e)
            {
                Conduit::connectAB(e, mux, key);
                return false;
            }
        }
        return true;
    }
    int getErrorCode() const
    {
        return code;
    }
};

class SocketUninstaller : public Visitor
{
    Socket* socket;

public:
    SocketUninstaller(Socket* s) :
        Visitor(s),
        socket(s)
    {
    }

    bool at(Adapter* a, Conduit* c)
    {
        return true;
    }

    bool at(Protocol* p, Conduit* c)
    {
        if (socket->getProtocol() == p)
        {
            return false;   // To stop this visitor
        }

        c->setA(0);
        p->setB(0);
        c->release();
        return true;
    }

    bool at(Mux* mux, Conduit* c)
    {
        if (c->isEmpty())
        {
            c->setA(0);
            mux->removeB(mux->getKey(socket));
            c->release();
            return true;
        }
        return false;       // To stop this visitor
    }
};

class SocketDisconnector : public Visitor
{
    Socket*     socket;
    Conduit*    protocol;

public:
    SocketDisconnector(Socket* s) :
        Visitor(s),
        socket(s),
        protocol(0)
    {
    }

    bool at(Protocol* p, Conduit* c)
    {
        if (socket->getProtocol() == p)
        {
            return false;   // To stop this visitor
        }

        ASSERT(protocol == 0);
        protocol = p;
        return true;
    }

    bool at(Mux* mux, Conduit* c)
    {
        if (c->isEmpty() || c == protocol)
        {
            c->setA(0);
            mux->removeB(mux->getKey(socket));
            if (c != protocol)
            {
                c->release();
            }
            return true;
        }
        return false;       // To stop this visitor
    }

    Conduit* getProtocol() const
    {
        return protocol;
    }
};

class SocketConnector : public Visitor
{
    Socket*     socket;
    int         code;
    Conduit*    protocol;

public:
    SocketConnector(Socket* s, Conduit* p) :
        Visitor(s),
        socket(s),
        code(0),
        protocol(p)
    {
    }
    bool at(Adapter* a, Conduit* c)
    {
        if (a->getReceiver() != socket)
        {
            code = EADDRINUSE;
        }
        return true;
    }
    bool at(ConduitFactory* f, Conduit* c)
    {
        Mux* mux = dynamic_cast<Mux*>(c);
        if (mux)
        {
            ASSERT(mux->getFactory() == f);
            void* key = mux->getKey(socket);
            if (dynamic_cast<InetRemoteAddressAccessor*>(mux->getAccessor()))
            {
                Conduit::connectAB(protocol, mux, key);
                return false;
            }
            else
            {
                Conduit* e = f->create(key);
                if (e)
                {
                    Conduit::connectAB(e, mux, key);
                    return false;
                }
            }
        }
        return true;
    }
    int getErrorCode() const
    {
        return code;
    }
};

#endif  // SOCKET_H_INCLUDED
