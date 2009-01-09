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

#include <ctype.h>
#include <errno.h>
#include <es.h>
#include <es/exception.h>
#include "alarm.h"
#include "cache.h"
#include "core.h"
#include "partition.h"
#include "process.h"
#include "thread.h"

int esReport(const char* spec, ...)
{
    va_list list;
    int count;

    va_start(list, spec);
    count = esReportv(spec, list);
    va_end(list);
    return count;
}

#ifdef __es__

// If malloc uses synchronization primitives other than Lock,
// the following monitorFactory lock type must also be changed.
static Lock monitorFactory;

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>

#ifdef PTHREAD_MUTEX_INITIALIZER

int pthread_once(pthread_once_t* once, void (*func) (void))
{
    if (!once->init_executed)
    {
        int initialized = 0;

        __asm__ __volatile__ (
            "xchgl  %0, %1\n"
            : "=a"(initialized), "=m"(once->is_initialized)
            : "0"(initialized), "m"(once->is_initialized));

        if (initialized)
        {
            if (func)
            {
                func();
            }
            once->init_executed = 1;
        }
        else
        {
            while (!once->init_executed)
            {
                Thread* current(Thread::getCurrentThread());
                if (current)
                {
                    current->sleep(10000);
                }
            }
        }
    }
    return 0;
}

static Monitor* getMonitor(pthread_mutex_t* mutex)
{
    Thread* current = Thread::getCurrentThread();
    if (!current)
    {
        return 0;
    }
    if (mutex->monitor == 0)
    {
        Lock::Synchronized method(monitorFactory);
        if (mutex->monitor == 0)
        {
            pthread_mutex_init(mutex, 0);
        }
    }
    return reinterpret_cast<Monitor*>(mutex->monitor);
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    Monitor* monitor = getMonitor(mutex);
    if (monitor)
    {
        monitor->lock();
    }
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    Monitor* monitor = getMonitor(mutex);
    if (monitor)
    {
        if (monitor->tryLock())
        {
            return 0;
        }
        else
        {
            return EBUSY;
        }
    }
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    Monitor* monitor = getMonitor(mutex);
    if (monitor)
    {
        monitor->unlock();
    }
    return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t* attr)
{
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type)
{
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t* attr)
{
    return 0;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
    Thread* current = Thread::getCurrentThread();
    if (!current)
    {
        mutex->monitor = 0;
        return 0;
    }
    mutex->monitor = (void*) new Monitor;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    if (mutex->monitor)
    {
        reinterpret_cast<Monitor*>(mutex->monitor)->release();
    }
    return 0;
}

#endif  // PTHREAD_MUTEX_INITIALIZER

#endif // __es__
