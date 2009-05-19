/*
 * Copyright 2008, 2009 Google Inc.
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

/*
 * These coded instructions, statements, and computer programs contain
 * software derived from Squeak.
 *
 *   Squeak is distributed for use and modification subject to a liberal
 *   open source license.
 *
 *   http://www.squeak.org/SqueakLicense/
 *
 *   Unless stated to the contrary, works submitted for incorporation
 *   into or for distribution with Squeak shall be presumed subject to
 *   the same license.
 *
 *   Portions of Squeak are:
 *
 *   Copyright (c) 1996 Apple Computer, Inc.
 *   Copyright (c) 1997-2001 Walt Disney Company, and/or
 *   Copyrighted works of other contributors.
 *   All rights reserved.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <es.h>
#include <es/endian.h>
#include <es/handle.h>
#include <es/dateTime.h>
#include <es/net/inet4.h>
#include <es/base/IProcess.h>
#include <es/naming/IContext.h>
#include <es/net/ISocket.h>
#include <es/net/IInternetConfig.h>
#include <es/net/IResolver.h>
#include "selector.h"

typedef unsigned long u_long;



es::CurrentProcess* System();

/* sqUnixSocket.c -- Unix socket support
 *
 *   Copyright (C) 1996-2005 by Ian Piumarta and other authors/contributors
 *                              listed elsewhere in this file.
 *   All rights reserved.
 *
 *   This file is part of Unix Squeak.
 *
 *      You are NOT ALLOWED to distribute modified versions of this file
 *      under its original name.  If you modify this file then you MUST
 *      rename it before making your modifications available publicly.
 *
 *   This file is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *   FITNESS FOR A PARTICULAR PURPOSE.
 *
 *   You may use and/or distribute this file ONLY as part of Squeak, under
 *   the terms of the Squeak License as described in `LICENSE' in the base of
 *   this distribution, subject to the following additional restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software.  If you use this software
 *      in a product, an acknowledgment to the original author(s) (and any
 *      other contributors mentioned herein) in the product documentation
 *      would be appreciated but is not required.
 *
 *   2. You must not distribute (or make publicly available by any
 *      means) a modified copy of this file unless you first rename it.
 *
 *   3. This notice must not be removed or altered in any source distribution.
 *
 *   Using (or modifying this file for use) in any context other than Squeak
 *   changes these copyright conditions.  Read the file `COPYING' in the
 *   directory `platforms/unix/doc' before proceeding with any such use.
 */

/* Author: Ian.Piumarta@inria.fr
 *
 * Last edited: 2005-03-09 02:24:15 by piumarta on squeak.hpl.hp.com
 *
 * Support for BSD-style "accept" primitives contributed by:
 *  Lex Spoon <lex@cc.gatech.edu>
 *
 * Notes:
 *  Sockets are completely asynchronous, but the resolver is still
 *  synchronous.
 *
 * BUGS:
 *  Now that the image has real UDP primitives, the TCP/UDP duality in
 *  many of the connection-oriented functions should be removed and
 *  cremated.
 */

extern "C"
{

#include "sq.h"
#include "SocketPlugin.h"

int synchronizedSignalSemaphoreWithIndex(int semaIndex);
}

#ifndef  HOST_NOT_FOUND
# define HOST_NOT_FOUND 1
#endif

#ifndef  NO_DATA
# define NO_DATA        4
#endif

/* Solaris sometimes fails to define this in netdb.h */
#ifndef  MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif

#ifdef NDEBUG
#define FPRINTF(...)    (__VA_ARGS__)
#else
#define FPRINTF(...)    esReport(__VA_ARGS__)
#endif

#define perror(X)       esReport(X)

/*** Socket types ***/

#define TCPSocketType           0
#define UDPSocketType           1


/*** Poll flags ***/

#define PollAccept              (1<<0)  // sqSocketListenOnPortBacklogSizeInterface
#define PollConnect             (1<<1)  // sqSocketConnectToPort
#define PollClose               (1<<2)  // sqSocketCloseConnection
#define PollReceive             (1<<3)  // sqSocketReceiveDataAvailable
#define PollSend                (1<<4)  // sqSocketSendDone


/*** Resolver states ***/

#define ResolverUninitialised   0
#define ResolverSuccess         1
#define ResolverBusy            2
#define ResolverError           3


/*** TCP Socket states ***/

#define Invalid                 -1
#define Unconnected             0
#define WaitingForConnection    1
#define Connected               2
#define OtherEndClosed          3
#define ThisEndClosed           4

#define LINGER_SECS             1

static int thisNetSession = 0;
static int one = 1;

static char   localHostName[MAXHOSTNAMELEN];
static u_long localHostAddress; /* GROSS IPv4 ASSUMPTION! */

struct privateSocketStruct
{
  Handle<es::Socket> s;              /* Unix socket */
  int connSema;                   /* connection io notification semaphore */
  int readSema;                   /* read io notification semaphore */
  int writeSema;                  /* write io notification semaphore */
  int sockState;                  /* connection + data state */
  int sockError;                  /* errno after socket error */
  unsigned poll;                  /* poll flags */
  Handle<es::InternetAddress> peer;  /* default send/recv address for UDP */
  int peerPort;                   /* default send/recv port for UDP */
  int multiListen;                /* whether to listen for multiple connections */
  Handle<es::Socket> acceptedSock;   /* a connection that has been accepted */
};


#define CONN_NOTIFY     (1<<0)
#define READ_NOTIFY     (1<<1)
#define WRITE_NOTIFY    (1<<2)

#define PING(S,EVT)                                             \
{                                                               \
  synchronizedSignalSemaphoreWithIndex((S)->EVT##Sema);         \
  FPRINTF("notify %p %s\n", (S)->s.get(), #EVT);                \
}

#define notify(SOCK,MASK)                                       \
{                                                               \
  if ((MASK) & CONN_NOTIFY)  PING(SOCK,conn);                   \
  if ((MASK) & READ_NOTIFY)  PING(SOCK,read);                   \
  if ((MASK) & WRITE_NOTIFY) PING(SOCK,write);                  \
}


/*** Accessors for private socket members from a Squeak socket pointer ***/

#define _PSP(S)     (((S)->privateSocketPtr))
#define PSP(S)      ((privateSocketStruct *)((S)->privateSocketPtr))

#define SOCKET(S)           (PSP(S)->s)
#define SOCKETSTATE(S)      (PSP(S)->sockState)
#define SOCKETERROR(S)      (PSP(S)->sockError)
#define SOCKETPEER(S)       (PSP(S)->peer)
#define SOCKETPEERPORT(S)   (PSP(S)->peerPort)


/*** Resolver state ***/

static char lastName[MAXHOSTNAMELEN+1];
static int  lastAddr= 0;
static int  lastError= 0;
static int  resolverSema= 0;

/*** Variables ***/

static Selector* selector;

static Handle<es::Resolver> resolver;
static char resolverBuffer[MAXHOSTNAMELEN];

/*** module initialisation/shutdown ***/


int socketInit(void)
{
  esReport("socketInit()\n");
  return 1;
}

int socketShutdown(void)
{
  /* shutdown the network */
  sqNetworkShutdown();
  return 1;
}


/***      miscellaneous sundries           ***/

/* answer the hostname for the given IP address */

static const char *addrToName(int netAddress)
{
  lastError= 0;         /* for the resolver */

  InAddr saddr= { htonl(netAddress) };
  Handle<es::InternetAddress> inetAddr = resolver->getHostByAddress(&saddr.addr, sizeof(saddr), 0);
  if (inetAddr && (0 <= inetAddr->getHostName(resolverBuffer, MAXHOSTNAMELEN)))
  {
    return resolverBuffer;
  }
  lastError= NO_DATA;
  return "";
}

/* answer the IP address for the given hostname */

static int nameToAddr(char *hostName)
{
  lastError= 0;         /* ditto */

  if (Handle<es::InternetAddress> address = resolver->getHostByName(hostName, AF_INET))
  {
    InAddr saddr;
    address->getAddress(&saddr.addr, sizeof(saddr));
    return ntohl(saddr.addr);
  }
  lastError= HOST_NOT_FOUND;        /* and one more ditto */
  return 0;
}

/* answer whether the given socket is valid in this net session */

static int socketValid(SocketPtr s)
{
  if (s && s->privateSocketPtr && thisNetSession && (s->sessionID == thisNetSession))
    return true;
  success(false);
  return false;
}

/* answer 1 if the given socket is readable,
          0 if read would block, or
         -1 if the socket is no longer connected */

static int socketReadable(es::Socket* s)
{
  if (s->isReadable())
    {
      return s->isClosed() ? -1 : 1;
    }
  else
    {
      return 0;
    }
}


/* answer whether the socket can be written without blocking */

static int socketWritable(es::Socket* s)
{
  if (s->isWritable())
    {
      return s->isClosed() ? -1 : 1;
    }
  else
    {
      return 0;
    }
}

/* answer the error condition on the given socket */

static int socketError(es::Socket* socket)
{
  return socket->getLastError();
}


/***     asynchronous io handlers       ***/


/* accept() can now be performed for the socket: call accept(),
   and replace the server socket with the new client socket
   leaving the client socket unhandled
*/
static void acceptHandler(void *data)
{
  privateSocketStruct *pss= (privateSocketStruct *)data;
  Handle<es::Socket> socket = pss->s;
  FPRINTF("acceptHandler(%p, %p, %d)\n", socket.get(), data, socket->isAcceptable());
  if (socket->isAcceptable())
    {
      if (socket->isClosed()) /* -- exception */
        {
          /* error during listen() */
          socket->setBlocking(true); // stop listening.
          pss->sockError= socketError(socket);
          pss->sockState= Invalid;
          pss->s= 0;
          Handle<es::Selectable> selectable(socket);
          selector->remove(selectable);
          socket->close();
          esReport("acceptHandler: aborting server %p pss=%p\n", socket.get(), pss);
          notify(pss, CONN_NOTIFY);
        }
      else
        {
          es::Socket* newSock= socket->accept();
          if (newSock) /* -- connection accepted */
            {
              pss->sockState= Connected;
              if (pss->multiListen)
                {
                  pss->acceptedSock= newSock;
                  // continue listening.
                }
              else /* traditional listen -- replace server with client in-place */
                {
                  socket->setBlocking(true);
                  Handle<es::Selectable> selectable(socket);
                  selector->remove(selectable);
                  socket->close();
                  pss->s= newSock;
                  newSock->setBlocking(false);
                }
              notify(pss, CONN_NOTIFY);
            }
        }
    }
}


/* connect() has completed: check errors, leaving the socket unhandled */

static void connectHandler(void *data)
{
  privateSocketStruct *pss= (privateSocketStruct *)data;
  Handle<es::Socket> socket = pss->s;
  FPRINTF("connectHandler(%p, %p, %d)\n", socket.get(), data, socket->isConnectable());
  if (socket->isConnectable())
  {
    if (socket->isClosed()) /* -- exception */
      {
        /* error during asynchronous connect() */
        socket->setBlocking(true);   // stop connecting.
        pss->sockError= socketError(socket);
        pss->sockState= Unconnected;
        perror("connectHandler");
      }
    else /* -- connect completed */
      {
        /* connect() has completed */
        int error= socketError(socket);
        if (error)
          {
            FPRINTF("connectHandler: error %d (%s)\n", error, strerror(error));
            pss->sockError= error;
            pss->sockState= Unconnected;
          }
            else
          {
            pss->sockState= Connected;
          }
      }
    // Handle<es::Selectable> selectable(socket);
    // selector->remove(selectable);
    pss->poll &= ~PollConnect;
    notify(pss, CONN_NOTIFY);
  }
}


/* read data transfer is now possible for the socket. */

static void readHandler(void *data)
{
  privateSocketStruct *pss= (privateSocketStruct *)data;
  Handle<es::Socket> socket = pss->s;
//  FPRINTF("readHandler(%p, %p, %d)\n", socket.get(), data, socket->isReadable());

  // Handle<es::Selectable> selectable(socket);
  int n= socketReadable(socket);
  FPRINTF("readHandler(%p, %p, %d)\n", socket.get(), data, n);
  if (n == -1)
    {
      pss->sockError= socketError(socket);
      pss->sockState= OtherEndClosed;
      // selector->remove(selectable);
      pss->poll &= ~PollReceive;
    }
  else if (n == 1)
    {
      // selector->remove(selectable);
      pss->poll &= ~PollReceive;
      notify(pss, READ_NOTIFY);
    }
  FPRINTF("readHandler: done\n");

#if 0
  if (flags & AIO_X)
    {
      /* assume out-of-band data has arrived */
      /* NOTE: Squeak's socket interface is currently incapable of reading
       *       OOB data.  We have no choice but to discard it.  Ho hum. */
      char buf[1];
      int n;
      /* [check] not supported.
      n= socket->recv((void *)buf, 1, MSG_OOB);
      if (n == 1) esReport("socket: received OOB data: %02x\n", buf[0]);
      */
    }
#endif
}


/* write data transfer is now possible for the socket. */

static void writeHandler(void *data)
{
  privateSocketStruct *pss= (privateSocketStruct *)data;
  Handle<es::Socket> socket = pss->s;
  FPRINTF("writeHandler(%p, %p, %d)\n", socket.get(), data, socket->isWritable());

  // Handle<es::Selectable> selectable(socket);
  int n= socketWritable(socket);
  if (n == -1)
    {
      pss->sockError= socketError(socket);
      pss->sockState= ThisEndClosed;
      // selector->remove(selectable);
      pss->poll &= ~PollSend;
    }
  else if (n == 1)
    {
      // selector->remove(selectable);
      pss->poll &= ~PollSend;
      notify(pss, WRITE_NOTIFY);
    }
}


/* a non-blocking close() has completed -- finish tidying up */

static void closeHandler(void *data)
{
  privateSocketStruct *pss= (privateSocketStruct *)data;
  Handle<es::Socket> socket = pss->s;
  FPRINTF("closeHandler(%p, %p, %d)\n", socket.get(), data, socket->isClosed());
  socket->setBlocking(false);
  socket->setTimeout(0);
  socket->close();
  pss->sockState= Unconnected;
  pss->s= 0;
  Handle<es::Selectable> selectable(socket);
  selector->remove(selectable);
  notify(pss, CONN_NOTIFY);
}


static void pollHandler(void *data)
{
  privateSocketStruct *pss= (privateSocketStruct *)data;
  if (pss->poll & PollAccept)
  {
    acceptHandler(data);
  }
  if (pss->poll & PollConnect)
  {
    connectHandler(data);
  }
  if (pss->poll & PollReceive)
  {
    readHandler(data);
  }
  if (pss->poll & PollSend)
  {
    writeHandler(data);
  }
  if (pss->poll & PollClose)
  {
    closeHandler(data);
  }
}

/***     Squeak network functions        ***/


/* start a new network session */

int sqNetworkInit(int resolverSemaIndex)
{
  if (0 != thisNetSession)
    return 0;  /* already initialised */

  Handle<es::Context> context = System()->getRoot();
  resolver = context->lookup("network/resolver");

  Handle<es::CurrentThread> currentThread = System()->currentThread();
  Handle<es::InternetConfig> config = context->lookup("network/config");
  Handle<es::InternetAddress> localhost;
  for (int i = 0; !localhost && i < 20; ++i)
  {
    localhost = config->getAddress(2);   // Assume DIX
    currentThread->sleep(10000000);
  }

  if (!localhost)
  {
    return -1;
  }

  InAddr saddr;
  localhost->getAddress(&saddr.addr, sizeof(saddr));
  localHostAddress = ntohl(saddr.addr);
  esReport("host: %d.%d.%d.%d\n",
           (u8) (localHostAddress >> 24),
           (u8) (localHostAddress >> 16),
           (u8) (localHostAddress >> 8),
           (u8) localHostAddress);
  localhost->getHostName(localHostName, sizeof(localHostName));

  thisNetSession = DateTime::getNow().getTicks();
  if (0 == thisNetSession)
    thisNetSession= 1;  /* 0 => uninitialised */
  resolverSema= resolverSemaIndex;
  return 0;
}


/* terminate the current network session (invalidates all open sockets) */

void sqNetworkShutdown(void)
{
  thisNetSession= 0;
  resolverSema= 0;
  // XXX stop asynchronous processes here.
}



/***  Squeak Generic Socket Functions   ***/


/* create a new socket */

void sqSocketCreateNetTypeSocketTypeRecvBytesSendBytesSemaID
    (SocketPtr s, int netType, int socketType,
     int recvBufSize, int sendBufSize, int semaIndex)
{
  sqSocketCreateNetTypeSocketTypeRecvBytesSendBytesSemaIDReadSemaIDWriteSemaID
    (s, netType, socketType,recvBufSize, sendBufSize,
     semaIndex, semaIndex, semaIndex);
}

void sqSocketCreateNetTypeSocketTypeRecvBytesSendBytesSemaIDReadSemaIDWriteSemaID
    (SocketPtr s, int netType, int socketType,
     int recvBufSize, int sendBufSize,
     int semaIndex, int readSemaIndex, int writeSemaIndex)
{
  Handle<es::Socket> newSocket;
  privateSocketStruct *pss;
  Handle<es::InternetAddress> any = resolver->getHostByAddress(&InAddrAny.addr, sizeof(InAddr), 0);

  s->sessionID= 0;
  if (TCPSocketType == socketType)
    {
      /* --- TCP --- */
      newSocket = any->socket(AF_INET, es::Socket::Stream, 0);
    }
  else if (UDPSocketType == socketType)
    {
      /* --- UDP --- */
      Object* p = any->socket(AF_INET, es::Socket::Datagram, 0);
      newSocket = p;
    }
  if (!newSocket)
    {
      /* socket() failed, or incorrect socketType */
      success(false);
      return;
    }
  // newSocket->setSockOpt(SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));

  newSocket->setReceiveBufferSize(recvBufSize);
  newSocket->setSendBufferSize(sendBufSize);

  /* private socket structure */
  pss= new privateSocketStruct;
  if (pss == NULL)
    {
      esReport("acceptFrom: out of memory\n");
      success(false);
      return;
    }
  pss->s= newSocket;
  pss->connSema= semaIndex;
  pss->readSema= readSemaIndex;
  pss->writeSema= writeSemaIndex;

  /* UDP sockets are born "connected" */
  if (UDPSocketType == socketType)
    {
      pss->sockState= Connected;
      pss->s->setBlocking(false);
    }
  else
    {
      pss->sockState= Unconnected;
    }
  pss->sockError= 0;
  pss->poll = 0;
  /* initial UDP peer := wildcard */
  pss->peer = 0;    // XXX any ?
  pss->peerPort = 0;
  /* Squeak socket */
  s->sessionID= thisNetSession;
  s->socketType= socketType;
  s->privateSocketPtr= pss;
  FPRINTF("create(%p) -> %lx\n", SOCKET(s).get(), (unsigned long)PSP(s));
  /* Note: socket is in BLOCKING mode until setBlocking(false) is called for it! */
}


/* return the state of a socket */

int sqSocketConnectionStatus(SocketPtr s)
{
  if (!socketValid(s))
    return Invalid;
  /* we now know that the net session is valid, so if state is Invalid... */
  if (SOCKETSTATE(s) == Invalid)    /* see acceptHandler() */
    {
      esReport("socketStatus: freeing invalidated pss=%p\n", PSP(s));
      /*delete PSP(s);*/ /* this almost never happens -- safer not to free()?? */
      _PSP(s)= 0;
      success(false);
      return Invalid;
    }
#if 0
  /* check for connection closed by peer */
  if (SOCKETSTATE(s) == Connected)
    {
      int n= socketReadable(SOCKET(s));
      if (n < 0)
        {
          FPRINTF("socketStatus(%p): detected other end closed\n", SOCKET(s).get());
          SOCKETSTATE(s)= OtherEndClosed;
        }
    }
#endif
  FPRINTF("socketStatus(%p) -> %d\n", SOCKET(s).get(), SOCKETSTATE(s));
  return SOCKETSTATE(s);
}



/* TCP => start listening for incoming connections.
 * UDP => associate the local port number with the socket.
 */
void sqSocketListenOnPort(SocketPtr s, int port)
{
  sqSocketListenOnPortBacklogSize(s, port, 1);
}

void sqSocketListenOnPortBacklogSizeInterface(SocketPtr s, int port, int backlogSize, int addr)
{
  if (!socketValid(s))
    return;

  /* only TCP sockets have a backlog */
  if ((backlogSize > 1) && (s->socketType != TCPSocketType))
    {
      success(false);
      return;
    }

  PSP(s)->multiListen= (backlogSize > 1);
  FPRINTF("listenOnPortBacklogSize(%p, %d)\n", SOCKET(s).get(), backlogSize);

  InAddr saddr = { htonl(addr) };
  Handle<es::InternetAddress> address = resolver->getHostByAddress(&saddr.addr, sizeof(InAddr), 0);
  SOCKET(s)->bind(address, port);
  if (TCPSocketType == s->socketType)
    {
      /* --- TCP --- */
      SOCKET(s)->listen(backlogSize);
      SOCKETSTATE(s) = WaitingForConnection;
      SOCKET(s)->setBlocking(false);
      PSP(s)->poll |= PollAccept;   /* R => accept() */
      Handle<es::Selectable> selectable(SOCKET(s));
      selector->add(selectable, pollHandler, (void*) PSP(s));
    }
  else
    {
      /* --- UDP --- */
    }
}

void sqSocketListenOnPortBacklogSize(SocketPtr s, int port, int backlogSize)
{
  sqSocketListenOnPortBacklogSizeInterface(s, port, backlogSize, INADDR_ANY);
}

/* TCP => open a connection.
 * UDP => set remote address.
 */
void sqSocketConnectToPort(SocketPtr s, int addr, int port)
{
  if (!socketValid(s))
    return;
  FPRINTF("connectTo(%p)\n", SOCKET(s).get());
  InAddr saddr = { htonl(addr) };
  Handle<es::InternetAddress> address = resolver->getHostByAddress(&saddr.addr, sizeof(saddr), 0);

  if (UDPSocketType == s->socketType)
    {
      /* --- UDP --- */
      if (SOCKET(s))
        {
          SOCKETPEER(s) = address;
          SOCKETPEERPORT(s) = port;
          SOCKETSTATE(s)= Connected;
        }
    }
  else
    {
      /* --- TCP --- */
      SOCKET(s)->setBlocking(false);
      SOCKET(s)->connect(address, port);
      int result = socketError(SOCKET(s));
      if (result == 0)
        {
          /* connection completed synchronously */
          SOCKETSTATE(s)= Connected;
          notify(PSP(s), CONN_NOTIFY);
        }
      else
        {
          if (result == EINPROGRESS || result == EWOULDBLOCK)
            {
              /* asynchronous connection in progress */
              SOCKETSTATE(s)= WaitingForConnection;
              Handle<es::Selectable> selectable(SOCKET(s));
              PSP(s)->poll |= PollConnect;  /* W => connect() */
              selector->add(selectable, pollHandler, (void*) PSP(s));  /* W => connect() */
            }
          else
            {
              /* connection error */
              perror("sqConnectToPort");
              SOCKETSTATE(s)= Unconnected;
              SOCKETERROR(s)= result;
              notify(PSP(s), CONN_NOTIFY);
            }
        }
    }
}


void sqSocketAcceptFromRecvBytesSendBytesSemaID
    (SocketPtr s, SocketPtr serverSocket,
     int recvBufSize, int sendBufSize, int semaIndex)
{
  sqSocketAcceptFromRecvBytesSendBytesSemaIDReadSemaIDWriteSemaID
    (s, serverSocket, recvBufSize, sendBufSize,
     semaIndex, semaIndex, semaIndex);
}


void sqSocketAcceptFromRecvBytesSendBytesSemaIDReadSemaIDWriteSemaID
    (SocketPtr s, SocketPtr serverSocket,
     int recvBufSize, int sendBufSize,
     int semaIndex, int readSemaIndex, int writeSemaIndex)
{
  /* The image has already called waitForConnection, so there is no
     need to signal the server's connection semaphore again. */

  struct privateSocketStruct *pss;

  FPRINTF("acceptFrom(%p, %p)\n", s, SOCKET(serverSocket).get());

  /* sanity checks */
  if (!socketValid(serverSocket) || !PSP(serverSocket)->multiListen)
    {
      FPRINTF("accept failed: (multi->%d)\n", PSP(serverSocket)->multiListen);
      success(false);
      return;
    }

  /* check that a connection is there */
  if (!PSP(serverSocket)->acceptedSock)
    {
      esReport("acceptFrom: no socket available\n");
      success(false);
      return;
    }

  /* got connection -- fill in the structure */
  s->sessionID= 0;
  pss = new privateSocketStruct;
  if (pss == NULL)
    {
      esReport("acceptFrom: out of memory\n");
      success(false);
      return;
    }

  _PSP(s)= pss;
  pss->s= PSP(serverSocket)->acceptedSock;
  PSP(serverSocket)->acceptedSock= 0;
  SOCKETSTATE(serverSocket)= WaitingForConnection;
  s->sessionID= thisNetSession;
  pss->connSema= semaIndex;
  pss->readSema= readSemaIndex;
  pss->writeSema= writeSemaIndex;
  pss->sockState= Connected;
  pss->sockError= 0;
  SOCKET(s)->setBlocking(false);
}


/* close the socket */

void sqSocketCloseConnection(SocketPtr s)
{
  int result= 0;

  if (!socketValid(s))
    return;

  FPRINTF("closeConnection(%p)\n", SOCKET(s).get());

  if (!SOCKET(s))
    return; /* already closed */

  SOCKET(s)->setBlocking(false);
  SOCKET(s)->setTimeout(300000000L);
  SOCKETSTATE(s)= ThisEndClosed;
  SOCKET(s)->close();
  int err = socketError(SOCKET(s));
  if (0 == err)
    {
      /* close completed synchronously */
      SOCKETSTATE(s)= Unconnected;
      FPRINTF("closeConnection: disconnected\n");
      SOCKET(s) = 0;
    }
  else if (err != EWOULDBLOCK)
    {
      /* error */
      SOCKETSTATE(s)= Unconnected;
      SOCKETERROR(s)= err;
      notify(PSP(s), CONN_NOTIFY);
      perror("closeConnection");
    }
  else
    {
      /* asynchronous close in progress */
      SOCKETSTATE(s)= ThisEndClosed;
      Handle<es::Selectable> selectable;
      PSP(s)->poll |= PollClose;
      selector->add(selectable, pollHandler, (void*) PSP(s));  /* => close() done */
      FPRINTF("closeConnection: deferred [aioHandle is set]\n");
    }
}


/* close the socket without lingering */

void sqSocketAbortConnection(SocketPtr s)
{
  FPRINTF("abortConnection(%p)\n", SOCKET(s).get());
  if (!socketValid(s))
    return;
  SOCKET(s)->setBlocking(false);
  SOCKET(s)->setTimeout(0);
  SOCKETSTATE(s)= ThisEndClosed;
  SOCKET(s)->close();
  SOCKETSTATE(s)= Unconnected;
  FPRINTF("closeConnection: disconnected\n");
  SOCKET(s) = 0;
}


/* Release the resources associated with this socket.
   If a connection is open, abort it. */

void sqSocketDestroy(SocketPtr s)
{
  if (!socketValid(s))
    return;

  FPRINTF("destroy(%p)\n", SOCKET(s).get());

  if (SOCKET(s))
    sqSocketAbortConnection(s);     /* close if necessary */

  if (PSP(s))
    delete PSP(s);                  /* release private struct */

  _PSP(s)= 0;
}


/* answer the OS error code for the last socket operation */

int sqSocketError(SocketPtr s)
{
  if (!socketValid(s))
    return -1;
  return SOCKETERROR(s);
}


/* return the local IP address bound to a socket */

int sqSocketLocalAddress(SocketPtr s)
{
  if (!socketValid(s))
    return -1;
  InAddr saddr;
  Handle<es::InternetAddress> address = SOCKET(s)->getLocalAddress();
  if (!address || !address->getAddress(&saddr.addr, sizeof(saddr)))
    return 0;
  return ntohl(saddr.addr);
}


/* return the peer's IP address */

int sqSocketRemoteAddress(SocketPtr s)
{
  if (!socketValid(s))
    return -1;

  Handle<es::InternetAddress> address;
  if (TCPSocketType == s->socketType)
    {
      /* --- TCP --- */
      address = SOCKET(s)->getRemoteAddress();
    }
  else
    {
      /* --- UDP --- */
      address = SOCKETPEER(s);
    }
  InAddr saddr;
  if (!address || !address->getAddress(&saddr.addr, sizeof(saddr)))
    return 0;
  return ntohl(saddr.addr);
}


/* return the local port number of a socket */

int sqSocketLocalPort(SocketPtr s)
{
  if (!socketValid(s))
    return -1;
  return SOCKET(s)->getLocalPort();
}


/* return the peer's port number */

int sqSocketRemotePort(SocketPtr s)
{
  if (!socketValid(s))
    return -1;
  if (TCPSocketType == s->socketType)
    {
      /* --- TCP --- */
      return SOCKET(s)->getRemotePort();
    }
  /* --- UDP --- */
  return SOCKETPEERPORT(s);
}


/* answer whether the socket has data available for reading:
   if the socket is not connected, answer "false";
   if the socket is open and data can be read, answer "true".
   if the socket is open and no data is currently readable, answer "false";
   if the socket is closed by peer, change the state to OtherEndClosed
    and answer "false";
*/
int sqSocketReceiveDataAvailable(SocketPtr s)
{
  if (!socketValid(s)) return false;
  if (SOCKETSTATE(s) == Connected)
    {
      int n= socketReadable(SOCKET(s));
      if (n > 0)
        {
          FPRINTF("receiveDataAvailable(%p) -> true\n", SOCKET(s).get());
          return true;
        }
      else if (n < 0)
        {
          FPRINTF("receiveDataAvailable(%p): other end closed\n", SOCKET(s).get());
          SOCKETSTATE(s)= OtherEndClosed;
        }
    }
  else /* (SOCKETSTATE(s) != Connected) */
    {
      FPRINTF("receiveDataAvailable(%p): socket not connected\n", SOCKET(s).get());
    }
  Handle<es::Selectable> selectable(SOCKET(s));
  PSP(s)->poll |= PollReceive;
  selector->add(selectable, pollHandler, (void*) PSP(s));
  FPRINTF("receiveDataAvailable(%p) -> false [aioHandle is set]\n", SOCKET(s).get());
  return false;
}


/* answer whether the socket has space to receive more data */

int sqSocketSendDone(SocketPtr s)
{
  if (!socketValid(s))
    return false;
  if (SOCKETSTATE(s) == Connected)
    {
      if (socketWritable(SOCKET(s)) == 1) return true;
      Handle<es::Selectable> selectable(SOCKET(s));
      PSP(s)->poll |= PollSend;
      selector->add(selectable, pollHandler, (void*) PSP(s));
    }
  return false;
}


/* read data from the socket s into buf for at most bufSize bytes.
   answer the number actually read.  For UDP, fill in the peer's address
   with the approriate value.
*/
int sqSocketReceiveDataBufCount(SocketPtr s, int buf, int bufSize)
{
  int nread= 0;

  if (!socketValid(s))
    return -1;
  if (UDPSocketType == s->socketType)
    {
      /* --- UDP --- */
      es::InternetAddress* peer;
      nread= SOCKET(s)->recvFrom((void *)buf, bufSize, 0, &peer, &SOCKETPEERPORT(s));
      if (nread <= 0)
        {
          int err = -nread; // XXX socketError(SOCKET(s));
          if (err == EWOULDBLOCK)
            {
              FPRINTF("UDP receiveData(%p) < 1 [blocked]\n", SOCKET(s).get());
              return 0;
            }
          SOCKETERROR(s)= err;
          FPRINTF("UDP receiveData(%p) < 1 [a:%d]\n", SOCKET(s).get(), err);
          return 0;
        }
      SOCKETPEER(s) = peer;
    }
  else
    {
      /* --- TCP --- */
      if ((nread= SOCKET(s)->read((void *)buf, bufSize)) <= 0)
        {
          int err = -nread; // XXX socketError(SOCKET(s));
          if (err == EWOULDBLOCK)
            {
              FPRINTF("TCP receiveData(%p) < 1 [blocked]\n", SOCKET(s).get());
              return 0;
            }
          /* connection reset */
          SOCKETSTATE(s)= OtherEndClosed;
          SOCKETERROR(s)= err;
          FPRINTF("TCP receiveData(%p) < 1 [b:%d]\n", SOCKET(s).get(), err);
          notify(PSP(s), CONN_NOTIFY);
          return 0;
        }
    }

  /* read completed synchronously */
  FPRINTF("receiveData(%p) done = %d\n", SOCKET(s).get(), nread);
  return nread;
}

/* write data to the socket s from buf for at most bufSize bytes.
   answer the number of bytes actually written.
*/
int sqSocketSendDataBufCount(SocketPtr s, int buf, int bufSize)
{
  int nsent= 0;

  if (!socketValid(s))
    return -1;

  if (UDPSocketType == s->socketType)
    {
      /* --- UDP --- */
      FPRINTF("UDP sendData(%p, %d)\n", SOCKET(s).get(), bufSize);
      nsent= SOCKET(s)->sendTo((void *)buf, bufSize, 0, SOCKETPEER(s), SOCKETPEERPORT(s));
      if (nsent<= 0)
        {
          int err = -nsent; // XXX socketError(SOCKET(s));
          if (err == EWOULDBLOCK) /* asynchronous write in progress */
            return 0;
          FPRINTF("UDP send failed\n");
          SOCKETERROR(s)= err;
          return 0;
        }
    }
  else
    {
      /* --- TCP --- */
      FPRINTF("TCP sendData(%p, %d)\n", SOCKET(s).get(), bufSize);
      if ((nsent= SOCKET(s)->write((char *)buf, bufSize)) <= 0)
        {
          int err = -nsent; // XXX socketError(SOCKET(s));
          if (err == EWOULDBLOCK)
            {
              FPRINTF("TCP sendData(%p, %d) -> %d [blocked]",
                      SOCKET(s).get(), bufSize, nsent);
              return 0;
            }
          else
            {
              /* error: most likely "connection closed by peer" */
              SOCKETSTATE(s)= OtherEndClosed;
              SOCKETERROR(s)= err;
              FPRINTF("TCP write failed -> %d", err);
              return 0;
            }
        }
    }
  /* write completed synchronously */
  FPRINTF("sendData(%p) done = %d\n", SOCKET(s).get(), nsent);
  return nsent;
}


/* read data from the UDP socket s into buf for at most bufSize bytes.
   answer the number of bytes actually read.
*/
int sqSocketReceiveUDPDataBufCountaddressportmoreFlag
    (SocketPtr s, int buf, int bufSize,  int *address,  int *port, int *moreFlag)
{
  if (socketValid(s) && (UDPSocketType == s->socketType))
    {
      FPRINTF("recvFrom(%p)\n", SOCKET(s).get());

      es::InternetAddress* from;
      int nread= SOCKET(s)->recvFrom((void *)buf, bufSize, 0, &from, port);
      if (nread >= 0)
        {
          InAddr saddr;
          from->getAddress(&saddr.addr, sizeof(saddr));
          *address = ntohl(saddr.addr);
          return nread;
        }
      int err = socketError(SOCKET(s));
      if (err == EWOULDBLOCK)   /* asynchronous read in progress */
        return 0;
      SOCKETERROR(s)= err;
      FPRINTF("receiveData(%p)= %da\n", SOCKET(s).get(), 0);
    }
  success(false);
  return 0;
}


/* write data to the UDP socket s from buf for at most bufSize bytes.
 * answer the number of bytes actually written.
 */
int sqSockettoHostportSendDataBufCount(SocketPtr s, int address, int port,
                       int buf, int bufSize)
{
  if (socketValid(s) && (UDPSocketType == s->socketType))
    {
      FPRINTF("sendTo(%p)\n", SOCKET(s).get());

      InAddr saddr = { htonl(address) };
      Handle<es::InternetAddress> to = resolver->getHostByAddress(&saddr.addr, sizeof(saddr), 0);

      int nsent= SOCKET(s)->sendTo((void *)buf, bufSize, 0, to, port);
      if (nsent >= 0)
        return nsent;

      int err = socketError(SOCKET(s));
      if (err == EWOULDBLOCK)   /* asynchronous write in progress */
        return 0;
      FPRINTF("UDP send failed\n");
      SOCKETERROR(s)= err;
    }
  success(false);
  return 0;
}


/*** socket options ***/


/* NOTE: we only support the portable options here as an incentive for
         people to write portable Squeak programs.  If you need
         non-portable socket options then go write yourself a plugin
         specific to your platform.  This decision is unilateral and
         non-negotiable.  - ikp
   NOTE: we only support the integer-valued options because the code
     in SocketPlugin doesn't seem able to cope with the others.
     (Personally I think that things like SO_SNDTIMEO et al would
     by far more interesting than the majority of things on this
     list, but there you go...)
   NOTE: if your build fails because of a missing option in this list,
     simply DELETE THE OPTION (or comment it out) and then send
     me mail (ian.piumarta@inria.fr) to let me know about it.
 */

typedef struct
{
  char *name;       /* name as known to Squeak */
  int   optlevel;   /* protocol level */
  int   optname;    /* name as known to Unix */
} socketOption;

#ifndef SOL_IP
# define SOL_IP IPPROTO_IP
#endif

#ifndef SOL_UDP
# define SOL_UDP IPPROTO_UDP
#endif

#ifndef SOL_TCP
# define SOL_TCP IPPROTO_TCP
#endif

static socketOption socketOptions[]= {
#if 0 // not supported.
  { "SO_DEBUG",             SOL_SOCKET, SO_DEBUG },
  { "SO_REUSEADDR",         SOL_SOCKET, SO_REUSEADDR },
  { "SO_DONTROUTE",         SOL_SOCKET, SO_DONTROUTE },
  { "SO_BROADCAST",         SOL_SOCKET, SO_BROADCAST },
  { "SO_SNDBUF",            SOL_SOCKET, SO_SNDBUF },
  { "SO_RCVBUF",            SOL_SOCKET, SO_RCVBUF },
  { "SO_KEEPALIVE",         SOL_SOCKET, SO_KEEPALIVE },
  { "SO_OOBINLINE",         SOL_SOCKET, SO_OOBINLINE },
  { "SO_LINGER",            SOL_SOCKET, SO_LINGER },
  { "IP_TTL",               SOL_IP,     IP_TTL },
  { "IP_HDRINCL",           SOL_IP,     IP_HDRINCL },
  { "IP_MULTICAST_IF",          SOL_IP,     IP_MULTICAST_IF },
  { "IP_MULTICAST_TTL",         SOL_IP,     IP_MULTICAST_TTL },
  { "IP_MULTICAST_LOOP",        SOL_IP,     IP_MULTICAST_LOOP },
#ifdef IP_ADD_MEMBERSHIP
  { "IP_ADD_MEMBERSHIP",        SOL_IP,     IP_ADD_MEMBERSHIP },
  { "IP_DROP_MEMBERSHIP",       SOL_IP,     IP_DROP_MEMBERSHIP },
#endif
  { "TCP_MAXSEG",           SOL_TCP,    TCP_MAXSEG },
  { "TCP_NODELAY",          SOL_TCP,    TCP_NODELAY },
# if 0 /*** deliberately unsupported options -- do NOT enable these! ***/
  { "SO_REUSEPORT",         SOL_SOCKET, SO_REUSEPORT },
  { "SO_PRIORITY",          SOL_SOCKET, SO_PRIORITY },
  { "SO_RCVLOWAT",          SOL_SOCKET, SO_RCVLOWAT },
  { "SO_SNDLOWAT",          SOL_SOCKET, SO_SNDLOWAT },
  { "IP_RCVOPTS",           SOL_IP,     IP_RCVOPTS },
  { "IP_RCVDSTADDR",            SOL_IP,     IP_RCVDSTADDR },
  { "UDP_CHECKSUM",         SOL_UDP,    UDP_CHECKSUM },
  { "TCP_ABORT_THRESHOLD",      SOL_TCP,    TCP_ABORT_THRESHOLD },
  { "TCP_CONN_NOTIFY_THRESHOLD",    SOL_TCP,    TCP_CONN_NOTIFY_THRESHOLD },
  { "TCP_CONN_ABORT_THRESHOLD",     SOL_TCP,    TCP_CONN_ABORT_THRESHOLD },
  { "TCP_NOTIFY_THRESHOLD",     SOL_TCP,    TCP_NOTIFY_THRESHOLD },
  { "TCP_URGENT_PTR_TYPE",      SOL_TCP,    TCP_URGENT_PTR_TYPE },
# endif
#endif // not supported.
  { (char *)0,              0,      0 }
};


static socketOption *findOption(char *name, size_t nameSize)
{
  if (nameSize < 32)
    {
      socketOption *opt= 0;
      char buf[32];
      buf[nameSize]= '\0';
      strncpy(buf, name, nameSize);
      for (opt= socketOptions; opt->name != 0; ++opt)
        if (!strcmp(buf, opt->name))
          return opt;
    }
  return 0;
}


/* set the given option for the socket.  the option comes in as a
 * String.  (why on earth we might think this a good idea eludes me
 * ENTIRELY, so... if the string doesn't smell like an integer then we
 * copy it verbatim, assuming it's really a ByteArray pretending to be
 * a struct.  caveat hackor.)
 */
int sqSocketSetOptionsoptionNameStartoptionNameSizeoptionValueStartoptionValueSizereturnedValue
    (SocketPtr s,int optionName, int optionNameSize,
     int optionValue, int optionValueSize, int *result)
{
  if (socketValid(s))
    {
      socketOption *opt= findOption((char *)optionName, (size_t)optionNameSize);
      if (opt != 0)
        {
          int   val= 0;
          char  buf[32];
          char *endptr;
          /* this is JUST PLAIN WRONG (I mean the design in the image rather
             than the implementation here, which is probably correct
             w.r.t. the broken design) */
          if (optionValueSize > sizeof(buf) - 1)
            goto barf;

          memset((void *)buf, 0, sizeof(buf));
          memcpy((void *)buf, (void *)optionValue, optionValueSize);
          if (optionValueSize == 1) /* character `1' or `0' */
            {
              val= strtol(buf, &endptr, 0);
              if (endptr != buf)
                {
                  memcpy((void *)buf, (void *)&val, sizeof(val));
                  optionValueSize= sizeof(val);
                }
            }
          int ret = -1;
          /* [check] not supported.
          ret = SOCKET(s)->setSockOpt(opt->optlevel, opt->optname,
                (const void *)buf, optionValueSize));
           */
          if (ret < 0)
            {
              perror("setsockopt");
              goto barf;
            }
          /* it isn't clear what we're supposed to return here, since
             setsockopt isn't supposed to have any value-result parameters
             (go grok that `const' on the buffer argument if you don't
             believe me).  the image says "the result of the negotiated
             value".  what the fuck is there to negotiate?  either
             setsockopt sets the value or it barfs.  and i'm not about to go
             calling getsockopt just to see if the value got changed or not
             (the image should send getOption: to the Socket if it really
             wants to know).  if the following is wrong then I could
             probably care (a lot) less...  fix the logic in the image and
             then maybe i'll care about fixing the logic in here.  (i know
             that isn't very helpful, but it's 05:47 in the morning and i'm
             severely grumpy after fixing several very unpleasant bugs that
             somebody introduced into this file while i wasn't looking.)  */
          *result= val;
          return 0;
        }
    }
 barf:
  success(false);
  return false;
}


/* query the socket for the given option.  */
int sqSocketGetOptionsoptionNameStartoptionNameSizereturnedValue
    (SocketPtr s,int optionName, int optionNameSize, int *result)
{
  if (socketValid(s))
    {
      socketOption *opt= findOption((char *)optionName, (size_t)optionNameSize);
      if (opt != 0)
        {
          int optval;
          int optlen= sizeof(optval);
          int ret = -1;
          /* [check] not supported.
          ret = SOCKET(s)->getSockOpt(opt->optlevel, opt->optname,
                  (void *)&optval, &optlen);
          */
          if (ret < 0)
            goto barf;
          if (optlen != sizeof(optval))
            goto barf;
          *result= optval;
          return 0;
        }
    }
 barf:
  success(false);
  return errno;
}


/*** Resolver functions ***/


/* Note: the Mac and Win32 implementations implement asynchronous lookups
 * in the DNS.  I can't think of an easy way to do this in Unix without
 * going totally ott with threads or somesuch.  If anyone knows differently,
 * please tell me about it. - Ian
 */


/*** irrelevancies ***/

void sqResolverAbort(void) {}

void sqResolverStartAddrLookup(int address)
{
  const char *res;
  res= addrToName(address);
  strncpy(lastName, res, MAXHOSTNAMELEN);
  FPRINTF("startAddrLookup %s\n", lastName);
}


int sqResolverStatus(void)
{
  if(!thisNetSession)
    return ResolverUninitialised;
  if(lastError != 0)
    return ResolverError;
  return ResolverSuccess;
}

/*** trivialities ***/

int sqResolverAddrLookupResultSize(void)    { return strlen(lastName); }
int sqResolverError(void)                   { return lastError; }
int sqResolverLocalAddress(void)            { return localHostAddress; }
int sqResolverNameLookupResult(void)        { return lastAddr; }

void sqResolverAddrLookupResult(char *nameForAddress, int nameSize)
{
  memcpy(nameForAddress, lastName, nameSize);
}

/*** name resolution ***/

void sqResolverStartNameLookup(char *hostName, int nameSize)
{
  int len= (nameSize < MAXHOSTNAMELEN) ? nameSize : MAXHOSTNAMELEN;
  memcpy(lastName, hostName, len);
  lastName[len]= lastError= 0;
  FPRINTF("name lookup %s\n", lastName);
  lastAddr= nameToAddr(lastName);
  /* we're done before we even started */
  synchronizedSignalSemaphoreWithIndex(resolverSema);
}

void* networkProcess(void* param)
{
    selector = new Selector();
    for (;;)
    {
        selector->wait(10000000);
    }
}
