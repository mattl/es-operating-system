/*
 * Copyright (c) 2006
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#include <es.h>
#include <es/classFactory.h>
#include <es/clsid.h>
#include <es/exception.h>
#include <es/endian.h>
#include <es/formatter.h>
#include <es/handle.h>
#include <es/base/IClassFactory.h>

#include "alarm.h"
#include "cache.h"
#include "classStore.h"
#include "context.h"
#include "partition.h"

#include <new>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace
{
    IContext*       root;
    IClassStore*    classStore;
    IClassFactory*  alarmFactory;
};

const int Page::SIZE = 4096;
const int Page::SHIFT = 12;
const int Page::SECTOR = 512;

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

    int err = pthread_key_create(&Thread::cleanupKey, Thread::cleanup);
    if (err)
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

    root = new Context;
    if (nameSpace)
    {
        *nameSpace = root;
    }

    classStore = static_cast<IClassStore*>(new ClassStore);
    IBinding* binding = root->bind("class", classStore);
    binding->release();

    // Register CLSID_CacheFactory
    IClassFactory* cacheFactoryFactory = new(ClassFactory<CacheFactory>);
    classStore->add(CLSID_CacheFactory, cacheFactoryFactory);

    // Register CLSID_MonitorFactory
    IClassFactory* monitorFactory = new(ClassFactory<Monitor>);
    classStore->add(CLSID_Monitor, monitorFactory);

    // Initialize the page table
    int fd = open("/dev/zero", O_RDWR);
    size_t size = 64 * 1024;
    void* arena = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
#ifdef VERBOSE
    esReport("arena: %p, size: %zu\n", arena, size);
#endif
    PageTable::init(arena, size);

    // Register CLSID_PageSet
    classStore->add(CLSID_PageSet, static_cast<IClassFactory*>(PageTable::pageSet));

    // Register CLSID_Alarm
    alarmFactory = new(ClassFactory<Alarm>);
    classStore->add(CLSID_Alarm, alarmFactory);

    // Register CLSID_Partition
    IClassFactory* partitionFactory = new(ClassFactory<PartitionContext>);
    classStore->add(CLSID_Partition, partitionFactory);

    return 0;
}

int esReportv(const char* spec, va_list list)
{
    Formatter formatter((int (*)(int, void*)) putc, stdout);
    return formatter.format(spec, list);
}

bool esCreateInstance(const Guid& rclsid, const Guid& riid, void** objectPtr)
{
    return classStore->createInstance(rclsid, riid, objectPtr);
}

void esSleep(s64 timeout)
{
    struct timespec ts;
    Thread* current(Thread::getCurrentThread());

    current->state = IThread::TIMED_WAITING;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout / 10000000;
    ts.tv_nsec += (timeout % 10000000) * 100;
    int err = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, 0);
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
