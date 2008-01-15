/*
 * Copyright 2008 Google Inc.
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
        addRef();
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
        addRef();
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
    release();
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

void* Monitor::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == IMonitor::iid())
    {
        objectPtr = static_cast<IMonitor*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<IMonitor*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
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
