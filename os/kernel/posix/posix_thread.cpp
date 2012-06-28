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
#include <string.h>
#include <unistd.h>

#include <es.h>
#include <es/exception.h>
#include "core.h"

pthread_key_t Thread::cleanupKey;
void (*Thread::dtorTable[MaxSpecific])(void*);

void* Thread::
init(void* arg)
{
    Thread* thread = static_cast<Thread*>(arg);

    int err = pthread_setspecific(cleanupKey, arg);
    if (err)
    {
        cleanup(arg);
        return 0;
    }
    return thread->run(thread->param);
}

void Thread::
cleanup(void* arg)
{
    Thread* thread = static_cast<Thread*>(arg);

    thread->state = TERMINATED;
    thread->release();
}

Thread::
Thread(void* (*run)(void*), void* param, int priority, void* stack, unsigned stackSize) :
    state(NEW), priority(priority), run(run), param(param), errorCode(0)
{
    memset(specific, 0, sizeof specific);
}

Thread::
~Thread()
{
    if (state != NEW)
    {
        pthread_detach(thread);
    }

    int key;
    for (key = 0; key < MaxSpecific; ++key)
    {
        const void* arg = specific[key];
        if (arg)
        {
            specific[key] = 0;
            if (dtorTable[key])
            {
                dtorTable[key](const_cast<void*>(arg));
            }
        }
    }
}

int Thread::
getState()
{
    return state;
}

int Thread::
getPriority()
{
    return priority;
}

void Thread::
setPriority(int priority)
{
    struct sched_param param;

    switch (getState())
    {
      case NEW:
      case TERMINATED:
        break;
      default:
        memset(&param, 0, sizeof param);
        param.sched_priority = priority;
        int err = pthread_setschedparam(thread, SCHED_RR, &param);
        switch (err)
        {
        case 0:
        case EPERM: // XXX
            break;
        default:
            esThrow(err);
        }
        break;
    }
    this->priority = priority;
}

void* Thread::
join()
{
    Thread* current = Thread::getCurrentThread();
    void* rval;

    switch (getState())
    {
      case NEW:
        return NULL;
      case TERMINATED:
        return const_cast<void*>(errorCode);
      default:
        addRef();
        current->state = es::Thread::WAITING;
        int err = pthread_join(thread, &rval);
        current->state = es::Thread::RUNNABLE;
        release();
        return (!err) ? rval : NULL;
    }
}

void Thread::
start()
{
    int                err;
    pthread_attr_t     attr;
    struct sched_param param;

    if (getState() != NEW)
    {
        throw SystemException<EALREADY>();
    }

    err = pthread_attr_init(&attr);
    if (err)
    {
        esThrow(err);
    }

    err = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    if (err)
    {
        pthread_attr_destroy(&attr);
        esThrow(err);
    }

    memset(&param, 0, sizeof param);
    param.sched_priority = priority;
    err = pthread_attr_setschedparam(&attr, &param);
    if (err)
    {
        pthread_attr_destroy(&attr);
        esThrow(err);
    }

    addRef();
    state = RUNNABLE;
    err = pthread_create(&thread, &attr, init, this);
    if (err)
    {
        release();
        pthread_attr_destroy(&attr);
        esThrow(err);
    }

    pthread_attr_destroy(&attr);
}

void Thread::
cancel()
{
    switch (getState())
    {
      case NEW:
      case TERMINATED:
        break;
      default:
        int err = pthread_cancel(thread);
        if (err)
        {
            esThrow(err);
        }
        state = TERMINATED;
        break;
    }
}

Object* Thread::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Thread::iid()) == 0)
    {
        objectPtr = static_cast<es::Thread*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Thread*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Thread::
addRef()
{
    return ref.addRef();
}

unsigned int Thread::
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

void Thread::
exit(const void* errorCode)
{
    this->errorCode = errorCode;
    pthread_exit(const_cast<void*>(errorCode));
}

void Thread::
setSpecific(int key, const void* ptr)
{
    if (key < 0 || MaxSpecific <= key)
    {
        throw SystemException<EINVAL>();
    }
    specific[key] = ptr;
}

void* Thread::
getSpecific(int key)
{
    if (key < 0 || MaxSpecific <= key)
    {
        throw SystemException<EINVAL>();
    }
    return const_cast<void*>(specific[key]);
}

Thread* Thread::
getCurrentThread()
{
    void* ptr = pthread_getspecific(cleanupKey);
    return static_cast<Thread*>(ptr);
}

void Thread::
reschedule()
{
}
