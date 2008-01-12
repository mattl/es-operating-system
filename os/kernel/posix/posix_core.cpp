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

// cf. http://www.opengroup.org/onlinepubs/007908799/xsh/pthread.h.html

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <es.h>
#include <es/dateTime.h>
#include <es/timeSpan.h>
#include <es/exception.h>
#include "core.h"

// ICurrentProcess

void Core::
exit(int status)
{
    ::exit(status);
}

void* Core::
map(const void* start, long long length, unsigned int prot, unsigned int flags, IPageable* pageable, long long offset)
{
    return 0;
}

void Core::
unmap(const void* start, long long length)
{
}

ICurrentThread* Core::
currentThread()
{
    addRef();
    return this;
}

IThread* Core::
createThread(void* (*run)(void*), void* param)
{
    return new Thread(run, param, IThread::Normal);
}

void Core::
yield()
{
}

IMonitor* Core::
createMonitor()
{
    return new Monitor;
}

IContext* Core::
getRoot()
{
    return 0;
}

IStream* Core::
getInput()
{
    return 0;
}

IStream* Core::
getOutput()
{
    return 0;
}

IStream* Core::
getError()
{
    return 0;
}

void* Core::
setBreak(long long increment)
{
    return sbrk(increment);
}

long long Core::
getNow()
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    DateTime now(1970, 1, 1);
    now += TimeSpan(ts.tv_sec * 10000000LL + ts.tv_nsec / 100);
    return now.getTicks();
}

void Core::
setStartup(void (*startup)(void* (*start)(void* param), void* param))
{
}

// ICurrentThread

void Core::
exit(const void* val)
{
    Thread* current(Thread::getCurrentThread());
    current->exit(val);
}

void Core::
sleep(long long timeout)
{
    struct timespec ts;

    ts.tv_sec = timeout / 10000000;
    ts.tv_nsec = (timeout % 10000000) * 100;
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, 0);
}

int Core::
setCancelState(int state)
{
    int err;
    int old;

    if (state & ICurrentThread::CANCEL_ENABLE)
    {
        err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE , &old);
    }
    else
    {
        err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
    }
    if (err)
    {
        esThrow(err);
    }
    switch (old)
    {
      case PTHREAD_CANCEL_ENABLE:
        old = ICurrentThread::CANCEL_ENABLE;
        break;
      case PTHREAD_CANCEL_DISABLE:
        old = ICurrentThread::CANCEL_DISABLE;
      default:
        old = 0;
        break;
    }
    return old;
}

int Core::
setCancelType(int type)
{
    int err;
    int old;

    if (type & ICurrentThread::CANCEL_ASYNCHRONOUS)
    {
        err = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old);
    }
    else
    {
        err = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old);
    }
    if (err)
    {
        esThrow(err);
    }
    switch (old)
    {
      case PTHREAD_CANCEL_ASYNCHRONOUS:
        old = ICurrentThread::CANCEL_ASYNCHRONOUS;
        break;
      case PTHREAD_CANCEL_DEFERRED:
        old = ICurrentThread::CANCEL_DEFERRED;
      default:
        old = 0;
        break;
    }
    return old;
}

void Core::
testCancel()
{
    pthread_testcancel();
}

void* Core::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == ICurrentThread::iid())
    {
        objectPtr = static_cast<ICurrentThread*>(this);
    }
    else if (riid == ICurrentProcess::iid())
    {
        objectPtr = static_cast<ICurrentProcess*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<ICurrentThread*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
}

unsigned int Core::
addRef(void)
{
    return ref.addRef();
}

unsigned int Core::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
