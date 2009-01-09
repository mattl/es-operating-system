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

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <map>
#include <new>

#include <es.h>
#include <es/rpc.h>

namespace es
{

__thread u8 RpcStack::rpcStackBase[RPC_STACK_SIZE];
__thread u8* RpcStack::rpcStack;

namespace
{

bool isMatch(int pid, const struct sockaddr_un* sa)
{
    char sun_path[108]; // UNIX_PATH_MAX

    memset(sun_path, 0, sizeof sun_path);
    sprintf(sun_path + 1, "es-socket-%u", pid);
    return (memcmp(sun_path, sa->sun_path, sizeof sun_path) == 0) ? true : false;
}

int* getRights(struct msghdr* msg, int* fdv, int maxfds)
{
    for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(msg);
         cmsg != 0;
         cmsg = CMSG_NXTHDR(msg, cmsg))
    {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
        {
            for (int i = 1; CMSG_LEN(i * sizeof(int)) <= cmsg->cmsg_len; ++i)
            {
                int fd = *((int*) CMSG_DATA(cmsg) + i - 1);
                if (i <= maxfds)
                {
                    *fdv++ = fd;
                }
                else
                {
                    close(fd);
                }
            }
        }
    }
    return fdv;
}

}   // namespace

struct sockaddr* getSocketAddress(int pid, struct sockaddr_un* sa)
{
    // Use the abstract namespace
    memset(sa, 0, sizeof(struct sockaddr_un));
    sa->sun_family = AF_UNIX;
    sprintf(sa->sun_path + 1, "es-socket-%u", pid);
    return reinterpret_cast<sockaddr*>(sa);
}

ssize_t receiveCommand(int s, CmdUnion* cmd, int flags)
{
    struct msghdr      msg;
    struct iovec       iov;
    int                maxfds = 0; // max. # of file descriptors to receive
    int*               fds;
    unsigned char      buf[CMSG_SPACE(8 * sizeof(int))];
    struct sockaddr_un sa;

    msg.msg_name = &sa;
    msg.msg_namelen = sizeof sa;
    iov.iov_base = cmd;
    iov.iov_len = sizeof(CmdUnion);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = CMSG_LEN(8 * sizeof(int));
    msg.msg_flags = 0;

    ssize_t rc = recvmsg(s, &msg, flags);
    if (rc == -1)
    {
        return -1;
    }

    if (rc < sizeof(CmdHdr) || !isMatch(cmd->hdr.pid, &sa))
    {
        errno = EBADMSG;
        rc = -1;
    }
    else
    {
        switch (cmd->cmd)
        {
        case CMD_CHAN_REQ:
            if (sizeof(CmdChanReq) != rc)
            {
                errno = EBADMSG;
                rc = -1;
                break;
            }
            fds = &cmd->chanReq.sockfd;
            maxfds = 1;
            break;
        case CMD_CHAN_RES:
            if (sizeof(CmdChanRes) != rc)
            {
                errno = EBADMSG;
                rc = -1;
                break;
            }
            break;
        case CMD_FORK_REQ:
            if (sizeof(CmdForkReq) != rc)
            {
                errno = EBADMSG;
                rc = -1;
                break;
            }
            break;
        case CMD_FORK_RES:
            if (sizeof(CmdForkRes) != rc)
            {
                errno = EBADMSG;
                rc = -1;
                break;
            }
            break;
        }
    }
    getRights(&msg, fds, maxfds);
    return rc;
}

RpcHdr* receiveMessage(int epfd, int* fdv, int*& fdmax, int* s)
{
    RpcHdr* hdr;
    unsigned char buf[CMSG_SPACE(8 * sizeof(int))];
    struct epoll_event event;

    // wait for the reply. note the thread might receive another request by recursive call, etc.
    for (;;)
    {
        int fdCount = epoll_wait(epfd, &event, 1, -1);
        if (fdCount == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE); // TODO: shoud not exit
        }
        if (fdCount != 1)
        {
            continue;
        }

        struct msghdr msg;
        struct iovec iov;
        msg.msg_name = 0;
        msg.msg_namelen = 0;
        iov.iov_base = RpcStack::top();
        iov.iov_len = RpcStack::getFreeSize();
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = buf;
        msg.msg_controllen = sizeof buf;
        msg.msg_flags = 0;
        int rc = recvmsg(event.data.fd, &msg, 0);
        if (0 <= rc)
        {
            fdmax = getRights(&msg, fdv, 8);
//            esDump(iov.iov_base, rc);
        }
        else
        {
            fdmax = fdv;
        }

        // TODO check message length msg.msg_iov[0].iov_len

        if (sizeof(RpcHdr) <= rc)
        {
#if 0
            fprintf(stderr, "%s:\n", __func__);
            esDump(iov.iov_base, rc);
#endif
            // TODO check trunk etc.
            hdr = reinterpret_cast<RpcHdr*>(iov.iov_base);
            if (hdr->cmd == RPC_REQ && sizeof(RpcReq) <= rc)
            {
                RpcStack::alloc(rc);
                break;
            }
            else if (hdr->cmd == RPC_RES && sizeof(RpcRes) <= rc)
            {
                RpcStack::alloc(rc);
                break;
            }
        }

        // Close unused rights
        for (int* p = fdv; p < fdmax; ++p)
        {
            close(*p);
        }
    }

#if 0
    std::map<int, int>::iterator it = socketMap->find(hdr->pid);
    if (it == socketMap->end())
    {
        (*socketMap)[hdr->pid] = event.data.fd;
    }
#endif

    *s = event.data.fd;
    return hdr;
}

}   // namespace es
