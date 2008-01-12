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

#include <new>
#include <string.h>
#include <stdlib.h>
#include <es.h>
#include <es/dateTime.h>
#include "core.h"
#include "thread.h"
#include "process.h"

Sched* Thread::sched;

void Thread::
setRun()
{
    sched->setRun(this);
}

void Thread::
unsetRun()
{
    sched->unsetRun(this);
}

bool Thread::
isAlive()
{
    return (NEW < state && state < TERMINATED) ? true : false;
}

bool Thread::
holdsLock(Monitor* monitor)
{
    bool holds = false;
    unsigned x = Core::splHi();
    lock();
    if (monitor)
    {
        Monitor* held;
        MonitorList::Iterator iter = monitorList.begin();
        while ((held = iter.next()))
        {
            if (held == monitor)
            {
                holds = true;
                break;
            }
        }
    }
    unlock();
    Core::splX(x);
    return holds;
}

int Thread::
getPriority()
{
    return base;
}

void Thread::
setPriority(int priority)
{
    if (priority < Lowest || Highest < priority)
    {
        return;
    }

    unsigned x = Core::splHi();
    if (base != priority)
    {
        base = priority;
        updatePriority();
    }
    reschedule();
    Core::splX(x);
}

/** Sets new effective scheduling priority to the thread.
 * @param effective New effective scheduling priority
 * @return          Pointer to thread to which the new scheduling
 *                  priority is to be propagated. NULL if no priority
 *                  propagation is required.
 */
Thread* Thread::
setEffectivePriority(int effective)
{
    switch (state)
    {
      case RUNNABLE:
        unsetRun();
        priority = effective;
        setRun();
        break;
      case WAITING:
        // Reorder threads in the queue
        rendezvous->update(this, effective);
        if (monitor)
        {
            // Alternatively we can just wake up monitor->owner.
            ASSERT(monitor->owner);
            return monitor->owner;
        }
        break;
      case RUNNING:
        sched->runQueueHint = true;    // Hint to scheduler to check run queue
        priority = effective;
        break;
      default:
        break;
    }
    return 0;
}

/** Updates the thread's scheduling priority based on the currently holding
 * monitor priorities.
 */
Thread* Thread::
resetPriority()
{
    ASSERT(isLocked());

    Thread* next;
    Monitor* monitor;
    MonitorList::Iterator iter = monitorList.begin();
    int effective = base;
    while ((monitor = iter.next()))
    {
        monitor->spinLock();
        int monitorPriority = monitor->getPriority();
        if (effective < monitorPriority)
        {
            effective = monitorPriority;
        }
    }
    if (priority != effective)
    {
        next = setEffectivePriority(effective);
    }
    else
    {
        next = 0;
    }
    while ((monitor = iter.previous()))
    {
        monitor->spinUnlock();
    }
    return next;
}

/** Updates the thread's scheduling priority. Propagates it if necessary.
 */
void Thread::
updatePriority()
{
    Thread* thread = this;
    do {
        thread->lock();
        Thread* next = thread->resetPriority();
        thread->unlock();
        // At this point, next might have been released the monitor.
        // XXX there is a small window where priority inversion could occur.
        thread = next;
    } while (thread);
}

/** Unlocks all the monitors locked by this thread.
 * This thread is being locked.
 */
void Thread::
unlockAllMonitors()
{
    ASSERT(state == TERMINATED);
    while (!monitorList.isEmpty())
    {
        Monitor* monitor = monitorList.getFirst();
        monitor->unlock();
    }
}

/** Checks deadlock condition
 * @return      true if this thread is in a deadlock condition.
 */
bool Thread::
isDeadlocked()
{
    Monitor* monitor;

    monitor = this->monitor;
    while (monitor && monitor->owner)
    {
        Thread* owner = monitor->owner;
        if (owner == this)
        {
            return true;
        }
        monitor = owner->monitor;
    }
    return false;
}

void Thread::
exit(void* val)
{
    UpcallRecord* record(upcallList.getLast());
    if (record)
    {
        // Move back to the client process.
        ASSERT(process);
        Ureg* ureg(static_cast<Ureg*>(param));
        ureg->ecx = reinterpret_cast<u32>(val);
        process->returnFromUpcall(ureg);
        return;
    }

    if (process)
    {
        process->detach(this);
    }

    unsigned x = Core::splHi();
    ASSERT(state == RUNNING);
    ASSERT(getCurrentThread() == this);

    lock();
    // XXX Clear FPU context
    state = TERMINATED;
    unlockAllMonitors();
    this->val = val;
    joinPoint.wakeup();
    unlock();

    reschedule();
    // NOT REACHED HERE

    Core::splX(x);
}

int Thread::
condJoin(int)
{
    return (state == TERMINATED) ? true : false;
}

bool Thread::
join(void** rval)
{
    DelegateTemplate<Thread> d(this, &Thread::condJoin);
    joinPoint.sleep(&d);
    if (state == TERMINATED)
    {
        if (rval)
        {
            *rval = const_cast<void*>(val); // Copy exit value
        }
        return true;
    }
    return false;
}

int Thread::
setCancelState(int state)
{
    unsigned x = Core::splHi();
    lock();

    // ASSERT(this == getCurrentThread());
    int previous = (attr & ICurrentThread::CANCEL_ENABLE);
    if (state & ICurrentThread::CANCEL_ENABLE)
    {
        attr |= ICurrentThread::CANCEL_ENABLE;
    }
    else
    {
        attr &= ~ICurrentThread::CANCEL_ENABLE;
    }

    unlock();
    Core::splX(x);

    return previous;
}

int Thread::
setCancelType(int type)
{
    unsigned x = Core::splHi();
    lock();

    // ASSERT(this == getCurrentThread());
    int previous = (attr & ICurrentThread::CANCEL_ASYNCHRONOUS);
    if (type & ICurrentThread::CANCEL_ASYNCHRONOUS)
    {
        attr |= ICurrentThread::CANCEL_ASYNCHRONOUS;
    }
    else
    {
        attr &= ~ICurrentThread::CANCEL_ASYNCHRONOUS;
    }

    unlock();
    Core::splX(x);

    return previous;
}

void Thread::
testCancel()
{
    unsigned x = Core::splHi();
    ASSERT(this == getCurrentThread());
    if (attr & ICurrentThread::CANCEL_REQUESTED)
    {
        exit(0);
    }
    Core::splX(x);
}

void Thread::
cancel()
{
    unsigned x = Core::splHi();
    lock();

    if (attr & ICurrentThread::CANCEL_DISABLE)
    {
        unlock();
        Core::splX(x);
        return;
    }

    attr |= ICurrentThread::CANCEL_REQUESTED;
    if (!(attr & ICurrentThread::CANCEL_ASYNCHRONOUS))
    {
        if (state == WAITING)
        {
            rendezvous->remove(this);
            state = IThread::RUNNABLE;
            setRun();
            if (monitor)
            {
                monitor->update();        // Reset BPI
                monitor = 0;
            }
        }

        unlock();
        Core::splX(x);
        return;
    }

    if (process)
    {
        process->detach(this);
    }

    switch (state)
    {
      case RUNNABLE:
        unsetRun();
        break;
      case RUNNING:
        sched->runQueueHint = true;    // Hint to scheduler to check run queue
        break;
      case WAITING:
        rendezvous->remove(this);
        if (monitor)
        {
            monitor->update();        // Reset BPI
            monitor = 0;
        }
        break;
      default:
        unlock();
        Core::splX(x);
        return;
    }

    // XXX Clear FPU context

    ASSERT(0 < ref);    // i.e., not detached.
    state = TERMINATED;
    unlockAllMonitors();
    joinPoint.wakeup();

    unlock();
    Core::splX(x);

    reschedule();
}

void Thread::
start()
{
    unsigned x = Core::splHi();
    lock();
    if (state == NEW)
    {
        state = RUNNABLE;
        setRun();
        addRef();
    }
    unlock();
    Core::splX(x);
    reschedule();
}

int Thread::
invoke(int)
{
    sleepPoint.wakeup();
    return 0;
}

int Thread::
condSleep(int)
{
    DateTime now(DateTime::getNow());
    return alarm.isExpired(now.getTicks());
}

void Thread::
sleep(s64 timeout)
{
    unsigned x = Core::splHi();
    ASSERT(state == RUNNING);
    alarm.setInterval(timeout);
    alarm.setCallback(static_cast<ICallback*>(this));
    alarm.setEnabled(true);
    DelegateTemplate<Thread> d(this, &Thread::condSleep);
    sleepPoint.sleep(&d);
    Core::splX(x);
}

void Thread::
startUp(void* param)
{
    Core::splLo();
    Thread* thread = (Thread*) param;
    void* ret = thread->func(thread->param);
    thread->exit(ret);
}

Thread::
Thread(void* (*func)(void*), void* param, int priority,
       void* stack, unsigned stackSize) :
    state(NEW),
    attr(ICurrentThread::CANCEL_DEFERRED | ICurrentThread::CANCEL_ENABLE),
    core(0),
    base(priority),
    priority(priority),
    error(0),
    rval(0),
    val((void*) -1),
    monitor(0),
    func(func),
    param(param),
    stackSize(stackSize),
    process(0),
    userStack(0)
{
    ASSERT(0 < stackSize);
    ASSERT(Lowest <= priority && priority <= Highest);
    if (!stack)
    {
        stack = new u8[stackSize];
    }
    this->stack = stack;
    *(int*) stack = 0xa5a5a5a5;
    sp0 = (u32) stack + stackSize - 2048;   // 2048: default kernel TLS size
    memset((void*) sp0, 0, 2048);           // Clear TLS
    ktcb = static_cast<u8*>(stack) + stackSize - sizeof(void*);
    *(void**) ktcb = ktcb;
    label.init(stack, stackSize - 2048 /* default kernel TLS size */, startUp, this);
#ifdef VERBOSE
    esReport("Thread::Thread %p %p %d\n", this, stack, stackSize);
#endif
}

Thread::
~Thread()
{
#ifdef VERBOSE
    esReport("Thread::%s %p\n", __func__, this);
#endif
    delete[] (u8*) stack;
}

bool Thread::
checkStack()
{
    return *(int*) stack == 0xa5a5a5a5;
}

void* Thread::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == IThread::iid())
    {
        objectPtr = static_cast<IThread*>(this);
    }
    else if (riid == ICallback::iid())
    {
        objectPtr = static_cast<ICallback*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<IThread*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
}

unsigned int Thread::
addRef(void)
{
    return ref.addRef();
}

unsigned int Thread::
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

Process* Thread::
returnToClient()
{
    Process* client;

    unsigned x = Core::splHi();
    Core* core = Core::getCurrentCore();

    upcallList.removeLast();
    UpcallRecord* record(upcallList.getLast());
    if (!record)
    {
        core->tcb->tcb = tcb;
        client = process;
    }
    else
    {
        core->tcb->tcb = record->tcb;
        client = record->process;
        ASSERT(client);
    }

    if (client)
    {
        client->load();
    }

    Core::splX(x);

    return client;
}

Process* Thread::
leapIntoServer(UpcallRecord* record)
{
    Process* server;

    unsigned x = Core::splHi();
    Core* core = Core::getCurrentCore();

    upcallList.addLast(record);
    core->tcb->tcb = record->tcb;
    server = record->process;
    ASSERT(server);
    server->load();

    Core::splX(x);
    return server;
}
