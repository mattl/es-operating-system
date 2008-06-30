/*
 * Copyright 2008 Google Inc.
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
#include <es/classFactory.h>
#include <es/clsid.h>
#include <es/context.h>
#include <es/exception.h>
#include <es/endian.h>
#include <es/formatter.h>
#include <es/handle.h>
#include <es/base/IClassFactory.h>

#include "alarm.h"
#include "cache.h"
#include "classStore.h"
#include "loopback.h"
#include "partition.h"
#include "posix/tap.h"

#include <sys/mman.h>

namespace
{
    IContext*       root;
    IClassStore*    classStore;
    IClassFactory*  alarmFactory;
    u8              loopbackBuffer[64 * 1024];
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
    Thread* thread = new Thread(0, 0, IThread::Normal);
    thread->thread = pthread_self();
    thread->state = IThread::RUNNABLE;
    thread->setPriority(IThread::Normal);
    pthread_setspecific(Thread::cleanupKey, thread);
}

int esInit(IInterface** nameSpace)
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

    // Create class store
    classStore = static_cast<IClassStore*>(new ClassStore);

    // Register CLSID_MonitorFactory which is used by Context
    IClassFactory* monitorFactory = new(ClassFactory<Monitor>);
    classStore->add(CLSID_Monitor, monitorFactory);

    root = new Context;
    if (nameSpace)
    {
        *nameSpace = root;
    }

    // Create class name space
    IBinding* binding = root->bind("class", classStore);
    binding->release();

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

    // Register CLSID_CacheFactory
    IClassFactory* cacheFactoryFactory = new(ClassFactory<CacheFactory>);
    classStore->add(CLSID_CacheFactory, cacheFactoryFactory);

    // Register CLSID_PageSet
    classStore->add(CLSID_PageSet, static_cast<IClassFactory*>(PageTable::pageSet));

    // Register CLSID_Alarm
    alarmFactory = new(ClassFactory<Alarm>);
    classStore->add(CLSID_Alarm, alarmFactory);

    // Register CLSID_Partition
    IClassFactory* partitionFactory = new(ClassFactory<PartitionContext>);
    classStore->add(CLSID_Partition, partitionFactory);

    // Create device name space
    IContext* device = root->createSubcontext("device");

    // Register the loopback interface
    Loopback* loopback = new Loopback(loopbackBuffer, sizeof loopbackBuffer);
    device->bind("loopback", static_cast<IStream*>(loopback));

#ifdef __linux__
    // Register the Ethernet interface
    try
    {
        Tap* tap = new Tap("wlan0");
        device->bind("ethernet", static_cast<IStream*>(tap));
    }
    catch (...)
    {
    }
#endif  // __linux__

    device->release();

    // Create network name space
    IContext* network = root->createSubcontext("network");
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

void* esCreateInstance(const Guid& rclsid, const Guid& riid)
{
    return classStore->createInstance(rclsid, riid);
}

void esSleep(s64 timeout)
{
    struct timespec ts;
    Thread* current(Thread::getCurrentThread());

    current->state = IThread::TIMED_WAITING;
    ts.tv_sec = timeout / 10000000;
    ts.tv_nsec = (timeout % 10000000) * 100;
    int err = nanosleep(&ts, 0);
    current->state = IThread::RUNNABLE;
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

IThread* esCreateThread(void* (*start)(void* param), void* param)
{
    return new Thread(start, param, IThread::Normal);
}

IMonitor* esCreateMonitor()
{
    return new Monitor;
}
