/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

#include <new>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <es.h>
#include <es/context.h>
#include <es/exception.h>
#include <es/endian.h>
#include <es/formatter.h>
#include <es/handle.h>

#include "alarm.h"
#include "cache.h"
#include "loopback.h"
#include "posix/tap.h"

#include <sys/mman.h>

namespace
{
    es::Context* root;
    es::Context* classStore;
    u8        loopbackBuffer[64 * 1024];
};

const int Page::SIZE = 4096;
const int Page::SHIFT = 12;
const int Page::SECTOR = 512;

void esInitThread()
{
    if (int err = pthread_key_create(&Thread::cleanupKey, Thread::cleanup))
    {
        esThrow(err);
    }
    int maxPriority = sched_get_priority_max(SCHED_RR);
    int minPriority = sched_get_priority_min(SCHED_RR);
#ifdef VERBOSE
    esReport("Priority: %d - %d\n", minPriority, maxPriority);
#endif

    // Create default thread
    Thread* thread = new Thread(0, 0, es::Thread::Normal);
    thread->thread = pthread_self();
    thread->state = es::Thread::RUNNABLE;
    thread->setPriority(es::Thread::Normal);
    pthread_setspecific(Thread::cleanupKey, thread);

    // Initialize trivial constructors.
    Alarm::initializeConstructor();
    Monitor::initializeConstructor();
    PageSet::initializeConstructor();
}

int esInit(Object** nameSpace)
{
    if (root)
    {
        if (nameSpace)
        {
            *nameSpace = root;
        }
        return 0;
    }

    esInitThread();

    root = new Context;
    if (nameSpace)
    {
        *nameSpace = root;
    }

    // Create class name space
    es::Context* classStore = root->createSubcontext("class");

    // Register es::Monitor constructor
    classStore->bind(es::Monitor::iid(), es::Monitor::getConstructor());

    // Initialize the page table
    size_t size = 64 * 1024;
    void* arena = 0;
    int fd = open("/dev/zero", O_RDWR);
    if (fd != -1)
    {
        arena = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
        close(fd);
    }
    if (arena == 0 || arena == (void*) -1)
    {
        arena = sbrk(0);
        arena = (void*) (((unsigned long) arena + 4096 - 1) & ~(4096 - 1));
        sbrk((char*) arena - (char*) sbrk(0) + size);
    }
#ifdef VERBOSE
    esReport("arena: %p, size: %zu\n", arena, size);
#endif
    PageTable::init(arena, size);

    // Register es::Alarm constructor
    classStore->bind(es::Alarm::iid(), es::Alarm::getConstructor());

    // Register es::Cache constructor
    Cache::initializeConstructor();
    es::Cache::setConstructor(new Cache::Constructor);
    classStore->bind(es::Cache::iid(), es::Cache::getConstructor());

    // Register the global page set
    classStore->bind(es::PageSet::iid(), es::PageSet::getConstructor());

    // Create device name space
    es::Context* device = root->createSubcontext("device");

    // Register the loopback interface
    Loopback* loopback = new Loopback(loopbackBuffer, sizeof loopbackBuffer);
    device->bind("loopback", static_cast<es::Stream*>(loopback));

#ifdef __linux__
    // Register the Ethernet interface
    try
    {
        Tap* tap = new Tap("eth1");
        device->bind("ethernet", static_cast<es::Stream*>(tap));
    }
    catch (...)
    {
    }
#endif  // __linux__

    device->release();

    // Create network name space
    es::Context* network = root->createSubcontext("network");
    network->release();

    return 0;
}

int esReportv(const char* spec, va_list list)
{
    Formatter formatter((int (*)(int, void*)) putc, stdout);
    int len = formatter.format(spec, list);
    fflush(stdout);
    return len;
}

void esSleep(s64 timeout)
{
    struct timespec ts;
    Thread* current(Thread::getCurrentThread());

    current->state = es::Thread::TIMED_WAITING;
    ts.tv_sec = timeout / 10000000;
    ts.tv_nsec = (timeout % 10000000) * 100;
    int err = nanosleep(&ts, 0);
    current->state = es::Thread::RUNNABLE;
    if (err)
    {
        esThrow(err);
    }
}

void esPanic(const char* file, int line, const char* msg, ...)
{
    va_list marker;

    va_start(marker, msg);
    esReportv(msg, marker);
    va_end(marker);
    esReport(" in \"%s\" on line %d.\n", file, line);
    exit(EXIT_FAILURE);
}

es::Thread* esCreateThread(void* (*start)(void* param), void* param)
{
    return new Thread(start, param, es::Thread::Normal);
}

es::Monitor* esCreateMonitor()
{
    return new Monitor;
}
