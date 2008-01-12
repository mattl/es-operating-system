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

#include <new>
#include "thread.h"

Thread::Monitor::
Monitor() : lockCount(0), owner(0)
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

void Thread::Monitor::
update()
{
    unsigned x = splHi();         // Cling to the current core
    spinLock();
    if (owner)
    {
        owner->updatePriority();
    }
    spinUnlock();
    splX(x);
}

int Thread::Monitor::
condLock(int)
{
    // current is locked by sleep().
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

    current->monitor = this;
    owner->updatePriority();
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
    // current is locked by sleep().
    Thread* current = getCurrentThread();

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
    Thread* current = getCurrentThread();

    ASSERT(0 < lockCount);
    ASSERT(current == owner);
    --lockCount;
    if (0 < lockCount)
    {
        return false;
    }

    // Recaluculate the effective scheduling priority after
    // removing the monitor from the queue.
    current->lock();
    current->monitorList.remove(this);
    current->unlock();
    owner = 0;
    release();

    current->updatePriority();
    return true;
}

void Thread::Monitor::
unlock()
{
    DelegateTemplate<Monitor> d(this, &Thread::Monitor::condUnlock);
    rendezvous.wakeup(&d);
    reschedule();
}

int Thread::Monitor::
condWait(int)
{
    // current is locked by sleep().
    Thread* current = getCurrentThread();
    if (current == owner)
    {
        lockCount = 0;
        current->monitorList.remove(this);
        owner = 0;
        current->updatePriority();
        rendezvous.wakeup();
        return false;
    }
    return true;
}

bool Thread::Monitor::
wait()
{
    unsigned x = splHi();         // Cling to the current core

    Thread* current = getCurrentThread();
    ASSERT(current);
    ASSERT(current->state == IThread::RUNNING);
    if (owner != current)
    {
        splX(x);
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

    splX(x);
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
    unsigned x = splHi();

    Thread* current = getCurrentThread();
    ASSERT(current);
    ASSERT(current->state == IThread::RUNNING);
    current->alarm.setInterval(timeout);
    current->alarm.setEnabled(true);
    current->alarm.setCallback(static_cast<ICallback*>(this));

    wait();

    current->alarm.setEnabled(false);

    splX(x);
    return !current->condSleep(0);
}

void Thread::Monitor::
notify()
{
    unsigned x = splHi();         // Cling to the current core
    cv.wakeup();
    Thread* current = getCurrentThread();
    if (current && current->state == IThread::RUNNING)
    {
        reschedule();
    }
    splX(x);
}

void Thread::Monitor::
notifyAll()
{
    notify();
}

int Thread::Monitor::
getPriority()
{
    return rendezvous.getPriority();
}

bool Thread::Monitor::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IMonitor)
    {
        *objectPtr = static_cast<IMonitor*>(this);
    }
    else if (riid == IID_ICallback)
    {
        *objectPtr = static_cast<ICallback*>(this);
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

unsigned int Thread::Monitor::
addRef(void)
{
    return ref.addRef();
}

unsigned int Thread::Monitor::
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
