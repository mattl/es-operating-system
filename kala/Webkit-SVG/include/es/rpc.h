/*
 * Copyright 2008, 2009 Google Inc.
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

#ifndef GOOGLE_ES_RPC_H_INCLUDED
#define GOOGLE_ES_RPC_H_INCLUDED

#include <stdio.h>

#include <es/any.h>
#include <es/capability.h>
#include <es/types.h>

namespace es
{

struct ThreadCredential
{
    int                 pid;
    int                 tid;
    u64                 check;  // maybe optional
};

inline int operator==(const ThreadCredential& c1, const ThreadCredential& c2)
{
   return (c1.pid == c2.pid &&
           c1.tid == c2.tid &&
           c1.check == c2.check) ? true : false;
}

inline int operator<(const ThreadCredential& c1, const ThreadCredential& c2)
{
   return (c1.pid <= c2.pid && c1.tid < c2.tid) ? true : false;
}

// System Commands (even: request, odd: reply)
static const int CMD_CHAN_REQ = 0;
static const int CMD_CHAN_RES = 1;
static const int CMD_FORK_REQ = 2;
static const int CMD_FORK_RES = 3;

struct CmdHdr
{
    int                 cmd;
    int                 pid;
};

struct CmdChanReq
{
    int                 cmd;     // CMD_CHAN_REQ
    int                 pid;
    int                 sockfd;
    Capability          cap;
    ThreadCredential    tc;
};

struct CmdChanRes
{
    int                 cmd;     // CMD_CHAN_RES
    int                 pid;
    ThreadCredential    tc;
};

struct CmdForkReq
{
    int                 cmd;     // CMD_FORK_REQ
    int                 pid;
};

struct CmdForkRes
{
    int                 cmd;     // CMD_FORK_RES
    int                 pid;
    Capability          in;
    Capability          out;
    Capability          error;
    Capability          root;
    Capability          current;
    Capability          document;

    void report()
    {
        printf("CmdForkRes:\n");
        in.report();
        out.report();
        error.report();
        root.report();
        current.report();
        document.report();
    }
};

union CmdUnion
{
    int         cmd;
    CmdHdr      hdr;
    CmdChanReq  chanReq;
    CmdChanRes  chanRes;
    CmdForkReq  forkReq;
    CmdForkRes  forkRes;
};

static const int RPC_REQ = 0;
static const int RPC_RES = 1;

// RPC request header

struct RpcHdr
{
    int         cmd;
    int         tag;
    int         pid;
};

struct RpcReq
{
    int         cmd;
    int         tag;
    int         pid;
    Capability  capability;
    unsigned    methodNumber;
    unsigned    paramCount;
    // Param    argv[];
    // Data

    Any* getArgv()
    {
        return reinterpret_cast<Any*>(this + 1);
    }

    void* getData()
    {
        return reinterpret_cast<void*>(getArgv() + paramCount);
    }
};

// RPC response header
struct RpcRes
{
    int         cmd;
    int         tag;
    int         pid;
    unsigned    exceptionCode;
    Any         result;
    // Data

    void* getData()
    {
        return reinterpret_cast<void*>(this + 1);
    }
};

class RpcStack
{
    static const int RPC_STACK_SIZE = 1024 * 1024;
    static const int ALIGN = 1 << 3;
    static __thread u8 rpcStackBase[RPC_STACK_SIZE];
    static __thread u8* rpcStack;

    u8* base;

public:
    RpcStack()
    {
        base = rpcStack;
    }

    ~RpcStack()
    {
        // Restore rpcStack
        rpcStack = base;
    }

    static void init()
    {
        rpcStack = rpcStackBase;
    }

    static void* alloc(size_t size)
    {
        size += ALIGN - 1;
        size &= ~(ALIGN - 1);
        if (size <= getFreeSize())
        {
            void* p = rpcStack;
            rpcStack += size;
            return p;
        }
        return 0;
    }

    static void* free(size_t size)
    {
        size += ALIGN - 1;
        size &= ~(ALIGN - 1);
        if (rpcStackBase <= (rpcStack - size))
        {
            return rpcStack -= size;
        }
        return rpcStackBase;
    }

    static void* top()
    {
        return rpcStack;
    }

    static size_t getFreeSize()
    {
        return rpcStackBase + RPC_STACK_SIZE - rpcStack;
    }
};

struct sockaddr* getSocketAddress(int pid, struct sockaddr_un* sa);
ssize_t receiveCommand(int s, CmdUnion* cmd, int flags = 0);
RpcHdr* receiveMessage(int epfd, int* fdv, int*& fdmax, int* s);

void dump(const void* ptr, s32 len);

}   // namespace es

#endif  // GOOGLE_ES_RPC_H_INCLUDED
