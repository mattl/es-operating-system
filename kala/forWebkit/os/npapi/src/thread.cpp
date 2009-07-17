/*
 * Copyright 2008 Google Inc.
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
#include <sys/types.h>

#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include "bridge.h"

__thread Process::Thread* currentThread;

Process::Thread::
Thread(Process* process, int s) :
    process(process)
{
    epfd = epoll_create(1);
    if (epfd < 0)
    {
        perror("epoll_create");
        return;
    }
    accept(s);

    // TODO: can we close s here?

    if (pthread_create(&thread, NULL, focus, reinterpret_cast<void*>(this)) != 0)
    {
        perror("pthread_create");
        return;
    }
    pthread_detach(thread);
}

Process::Thread::
~Thread()
{
    // TODO close sockets
    pthread_cancel(thread);
    close(epfd);
}

// TODO: can we close s here?
int Process::Thread::
accept(int s)
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT;
    event.data.fd = s;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, s, &event) == -1)
    {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}
