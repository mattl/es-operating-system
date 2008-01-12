/*
 * Copyright (c) 2006, 2007
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
#include <time.h>
#include <es.h>
#include <es/exception.h>
#include "core.h"

Monitor::
Monitor()
{
    int                 err;
    pthread_mutexattr_t attr;

    err = pthread_mutexattr_init(&attr);
    if (err)
    {
        esThrow(err);
    }
    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (err)
    {
        pthread_mutexattr_destroy(&attr);
        esThrow(err);
    }
    err = pthread_mutex_init(&mutex, &attr);
    if (err)
    {
        pthread_mutexattr_destroy(&attr);
        esThrow(err);
    }
    err = pthread_cond_init(&cond, NULL);
    if (err)
    {
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&mutex);
        esThrow(err);
    }

    pthread_mutexattr_destroy(&attr);
    pthread_mutexattr_destroy(&attr);
}

Monitor::
~Monitor()
{
    if (ref == 0)
    {
        // As POSIX doesn't define behavior when attempting to destroy a locked mutex,
        // we do not destroy it as long as it is referenced.
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
    }
}

void Monitor::
lock()
{
    Thread* current(Thread::getCurrentThread());

    if (!tryLock())
    {
        current->state = IThread::BLOCKED;
        int err = pthread_mutex_lock(&mutex);
        if (err)
        {
            esThrow(err);
        }
    }
    current->state = IThread::RUNNABLE;
}

bool Monitor::
tryLock()
{
    int err = pthread_mutex_trylock(&mutex);
    switch (err)
    {
      case 0:
        return true;
      case EBUSY:
      case EAGAIN:
        return false;
    }
    esThrow(err);
}

void Monitor::
unlock()
{
    int err = pthread_mutex_unlock(&mutex);
    if (err)
    {
        esThrow(err);
    }
}

bool Monitor::
wait()
{
    Thread* current(Thread::getCurrentThread());

    current->state = IThread::WAITING;
    int err = pthread_cond_wait(&cond, &mutex);
    current->state = IThread::RUNNABLE;
    if (err)
    {
        esThrow(err);
    }
    return true;
}

bool Monitor::
wait(s64 timeout)
{
    struct timespec ts;
    Thread* current(Thread::getCurrentThread());

    int err;
    if (0 < timeout)
    {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout / 10000000;
        ts.tv_nsec += (timeout % 10000000) * 100;
        current->state = IThread::TIMED_WAITING;
        err = pthread_cond_timedwait(&cond, &mutex, &ts);
        current->state = IThread::RUNNABLE;
    }
    else
    {
        current->state = IThread::WAITING;
        err = pthread_cond_wait(&cond, &mutex);
        current->state = IThread::RUNNABLE;
    }
    switch (err)
    {
    case 0:
    case EINVAL:
        return true;
    case ETIMEDOUT:
        return false;
    default:
        esThrow(err);
    }
}

// notify() should only be called by a thread that is the owner of this monitor.
void Monitor::
notify()
{
    int err = pthread_cond_signal(&cond);
    if (err)
    {
        esThrow(err);
    }
}

void Monitor::
notifyAll()
{
    int err = pthread_cond_broadcast(&cond);
    if (err)
    {
        esThrow(err);
    }
}

bool Monitor::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IMonitor)
    {
        *objectPtr = static_cast<IMonitor*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IMonitor*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Monitor::
addRef(void)
{
    return ref.addRef();
}

unsigned int Monitor::
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
