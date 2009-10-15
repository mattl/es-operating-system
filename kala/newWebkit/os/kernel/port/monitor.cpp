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

#include <new>
#include "core.h"
#include "thread.h"

Thread::Monitor::
Monitor() :
    lockCount(0),
    owner(0)
{
    return;
}

void Thread::Monitor::
spinLock()
{
    rendezvous.lock();
}

void Thread::Monitor::
spinUnlock()
{
    rendezvous.unlock();
}

int Thread::Monitor::
getPriority()
{
    return rendezvous.getPriority();
}

void Thread::Monitor::
update()
{
    unsigned x = Core::splHi();         // Cling to the current core
    spinLock();
    Thread* thread = owner;
    if (thread)
    {
        thread->addRef();
    }
    spinUnlock();
    if (thread)
    {
        thread->updatePriority();
        thread->release();
    }
    Core::splX(x);
}

int Thread::Monitor::
condLock(int)
{
    // current is NOT locked by sleep().
Retry:
    Thread* current = getCurrentThread();
    current->monitor = 0;

    if (owner == 0)
    {
        ASSERT(lockCount == 0);
        owner = current;
        ++lockCount;
        current->monitorList.addLast(this);
        addRef();
        return true;
    }

    if (owner == current)
    {
        ++lockCount;
        return true;
    }

#ifdef VERBOSE
    esReport("%s: %d\n%p\n%p\n", __func__, __LINE__, owner->func, current->func);
#endif

    // Note we cannot call owner->updatePriority() since this monitor's
    // rendezvous is locked and owner->updatePriority() also tries to
    // lock this rendezvous.
    Thread* next;
    current->lock();
    int effective = getPriority();
    if (effective < current->priority)
    {
        effective = current->priority;
    }

    ASSERT(owner);
    if (owner->tryLock())
    {
        if (owner->priority < effective)
        {
            next = owner->setEffectivePriority(effective);
        }
        else
        {
            next = 0;
        }
        owner->unlock();
        current->monitor = this;
        current->unlock();
    }
    else
    {
        Thread* prec = owner;
        prec->addRef();
        current->unlock();
        spinUnlock();
        prec->wait();
        // Now owner may not be equal to prec.
        prec->release();
        spinLock();
        goto Retry;
    }

    if (next)
    {
        // next must inherit the new effective priority
        // and since next does not own this monitor, we can finally call:
        next->updatePriority();
    }

    ASSERT(!current->isDeadlocked());
    return false;
}

void Thread::Monitor::
lock()
{
    DelegateTemplate<Monitor> d(this, &Thread::Monitor::condLock);
    rendezvous.sleep(&d);
}

int Thread::Monitor::
condTryLock(int)
{
    // current is NOT locked by sleep().
    Thread* current = getCurrentThread();

    if (owner == 0)
    {
        ASSERT(lockCount == 0);
        owner = current;
        ++lockCount;
        current->lock();
        current->monitorList.addLast(this);
        current->unlock();
        addRef();
        return true;
    }

    if (owner == current)
    {
        ++lockCount;
        return true;
    }

    return true;    // never sleep
}

bool Thread::Monitor::
tryLock()
{
    DelegateTemplate<Monitor> d(this, &Thread::Monitor::condTryLock);
    rendezvous.sleep(&d);
    return (getCurrentThread() == owner) ? true : false;
}

int Thread::Monitor::
condUnlock(int)
{
    if (!owner)
    {
        return false;
    }
    ASSERT(0 < lockCount);

    // owner is locked and is being terminated.
    if (owner->state == TERMINATED)
    {
        lockCount = 0;
        owner->monitorList.remove(this);
        owner = 0;
        release();
        return true;
    }

    Thread* current = getCurrentThread();
    if (current == owner)
    {
        --lockCount;
        if (0 < lockCount)
        {
            return false;
        }

        // Recalculate the effective scheduling priority after
        // removing the monitor from the queue.
        current->lock();
        current->monitorList.remove(this);
        current->unlock();
        owner = 0;
        release();
        current->updatePriority();
        return true;
    }

    return false;
}

void Thread::Monitor::
unlock()
{
    unsigned x = Core::splHi();
    DelegateTemplate<Monitor> d(this, &Thread::Monitor::condUnlock);
    rendezvous.wakeup(&d);
    if (getCurrentThread()->state != TERMINATED)
    {
        reschedule();
    }
    Core::splX(x);
}

int Thread::Monitor::
condWait(int)
{
    // current is NOT locked by sleep().
    Thread* current = getCurrentThread();
    if (current == owner)
    {
        rendezvous.lock();
        lockCount = 0;
        owner = 0;
        rendezvous.unlock();

        current->lock();
        current->monitorList.remove(this);
        current->resetPriority();
        current->unlock();
        rendezvous.wakeup();
        return false;
    }
    return true;
}

bool Thread::Monitor::
wait()
{
    unsigned x = Core::splHi();         // Cling to the current core

    Thread* current = getCurrentThread();
    ASSERT(current);
    ASSERT(current->state == es::Thread::RUNNING);
    if (owner != current)
    {
        Core::splX(x);
        return false;
    }

    // Unlock the monitor no matter what.
    int realCount = lockCount;
    DelegateTemplate<Monitor> d(this, &Thread::Monitor::condWait);
    cv.sleep(&d);

    // Lock the monitor back and restore the lock count.
    lock();
    lockCount = realCount;
    release();
    ASSERT(0 < lockCount);

    Core::splX(x);
    return true;
}

int Thread::Monitor::
invoke(int)
{
    notify();
    return 0;
}

bool Thread::Monitor::
wait(s64 timeout)
{
    unsigned x = Core::splHi();

    Thread* current = getCurrentThread();
    ASSERT(current);
    ASSERT(current->state == es::Thread::RUNNING);

    bool expired(false);
    if (0 < timeout)
    {
        current->alarm.setInterval(timeout);
        current->alarm.setCallback(static_cast<es::Callback*>(this));
        current->alarm.setEnabled(true);
        wait();
        expired = current->condSleep(0);
        current->alarm.setEnabled(false);
    }
    else
    {
        wait();
    }

    Core::splX(x);
    return !expired;
}

void Thread::Monitor::
notify()
{
    unsigned x = Core::splHi();
    cv.wakeup();
    reschedule();
    Core::splX(x);
}

void Thread::Monitor::
notifyAll()
{
    notify();
}

Object* Thread::Monitor::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Monitor::iid()) == 0)
    {
        objectPtr = static_cast<es::Monitor*>(this);
    }
    else if (strcmp(riid, es::Callback::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Monitor*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Thread::Monitor::
addRef()
{
    return ref.addRef();
}

unsigned int Thread::Monitor::
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

es::Monitor* Thread::Monitor::
Constructor::createInstance()
{
    return new Monitor;
}

Object* Thread::Monitor::
Constructor::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Monitor::Constructor::iid()) == 0)
    {
        objectPtr = static_cast<es::Monitor::Constructor*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Monitor::Constructor*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Thread::Monitor::
Constructor::addRef()
{
    return 1;
}

unsigned int Thread::Monitor::
Constructor::release()
{
    return 1;
}

void Thread::Monitor::
initializeConstructor()
{
    // cf. -fthreadsafe-statics for g++
    static Constructor constructor;
    es::Monitor::setConstructor(&constructor);
}
