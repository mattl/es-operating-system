/*
 * Copyright 2012 Esrille Inc. 
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

// cf. http://www.opengroup.org/onlinepubs/007908799/xsh/pthread.h.html

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <es.h>
#include <es/dateTime.h>
#include <es/timeSpan.h>
#include <es/exception.h>
#include "core.h"

// es::CurrentProcess

void Core::
exit(int status)
{
    ::exit(status);
}

void* Core::
map(void* start, long long length, unsigned int prot, unsigned int flags, es::Pageable* pageable, long long offset)
{
    return 0;
}

void Core::
unmap(void* start, long long length)
{
}

es::CurrentThread* Core::
currentThread()
{
    addRef();
    return this;
}

es::Thread* Core::
createThread(void* (*run)(void*), void* param)
{
    return new Thread(run, param, es::Thread::Normal);
}

void Core::
yield()
{
}

es::Monitor* Core::
createMonitor()
{
    return new Monitor;
}

es::Context* Core::
getRoot()
{
    return 0;
}

es::Stream* Core::
getInput()
{
    return 0;
}

es::Stream* Core::
getOutput()
{
    return 0;
}

es::Stream* Core::
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
#ifdef __APPLE__
    struct timeval tv;

    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
    DateTime now(1970, 1, 1);
    now += TimeSpan(ts.tv_sec * 10000000LL + ts.tv_nsec / 100);
    return now.getTicks();
}

void Core::
setStartup(void (*startup)(void* (*start)(void* param), void* param))
{
}

// es::CurrentThread

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
    nanosleep(&ts, 0);
}

int Core::
setCancelState(int state)
{
    int err;
    int old;

    if (state & es::CurrentThread::CANCEL_ENABLE)
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
        old = es::CurrentThread::CANCEL_ENABLE;
        break;
      case PTHREAD_CANCEL_DISABLE:
        old = es::CurrentThread::CANCEL_DISABLE;
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

    if (type & es::CurrentThread::CANCEL_ASYNCHRONOUS)
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
        old = es::CurrentThread::CANCEL_ASYNCHRONOUS;
        break;
      case PTHREAD_CANCEL_DEFERRED:
        old = es::CurrentThread::CANCEL_DEFERRED;
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

Object* Core::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::CurrentThread::iid()) == 0)
    {
        objectPtr = static_cast<es::CurrentThread*>(this);
    }
    else if (strcmp(riid, es::CurrentProcess::iid()) == 0)
    {
        objectPtr = static_cast<es::CurrentProcess*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::CurrentThread*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Core::
addRef()
{
    return ref.addRef();
}

unsigned int Core::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
