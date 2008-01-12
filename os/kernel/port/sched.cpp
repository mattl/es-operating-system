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

#include <string.h> // ffs()
#include "thread.h"
#include "process.h"

Sched::
Sched() :
    runQueueBits(0),
    runQueueHint(false)
{
}

void Sched::
setRun(Thread* thread)
{
    lock();
    ASSERT(thread->state == IThread::RUNNABLE);
    Thread::Queue* queue = &runQueue[thread->priority];
    queue->addLast(thread);
    runQueueBits |= 0x80000000u >> thread->priority;
    runQueueHint = true;   // Hint to scheduler to check run queue
    unlock();

    // XXX IPI to notify the other processors that another thread becomes ready to run.
}

void Sched::
unsetRun(Thread* thread)
{
    lock();
    ASSERT(thread->state == IThread::RUNNABLE);
    Thread::Queue* queue = &runQueue[thread->priority];
    queue->remove(thread);
    if (queue->isEmpty())
    {
        runQueueBits &= ~(0x80000000u >> thread->priority);
    }
    unlock();
}

Thread* Sched::
selectThread()
{
    int priority;
    Thread::Queue* queue;
    Thread* next;

    for (;;)
    {
        do
        {
            unsigned x = Thread::splIdle();
            while (runQueueBits == 0)
            {
#ifdef __i386__
                __asm__ __volatile__ ("hlt\n");
#endif
            }
            Thread::splX(x);
        } while (runQueueBits == 0);

        lock();
        priority = IThread::Highest + 1 - ffs(runQueueBits);
        if (priority <= IThread::Highest && IThread::Lowest <= priority)
        {
            queue = &runQueue[priority];
            next = queue->getFirst();
            if (next->tryLock())
            {
                break;
            }
        }
        unlock();
    }

    ASSERT(next->priority == priority);
    queue->remove(next);
    if (queue->isEmpty())
    {
        runQueueBits &= ~(0x80000000u >> priority);
    }
    next->state = IThread::RUNNING;

    unlock();
    next->unlock();     // XXX check if we can unlock next now
    return next;
}

//
//  ICurrentThread
//

void Sched::
exit(void* val)
{
    Thread* current(Thread::getCurrentThread());
    current->exit(val);
}

void Sched::
sleep(long long timeout)
{
    Thread* current(Thread::getCurrentThread());
    current->sleep(timeout);
}

int Sched::
setCancelState(int state)
{
    Thread* current(Thread::getCurrentThread());
    return current->setCancelState(state);
}

int Sched::
setCancelType(int type)
{
    Thread* current(Thread::getCurrentThread());
    return current->setCancelType(type);
}

void Sched::
testCancel()
{
    Thread* current(Thread::getCurrentThread());
    current->testCancel();
}

int Sched::
getState()
{
    Thread* current(Thread::getCurrentThread());
    return current->getState();
}

//
// ICurrentProcess
//

void Sched::
exit(int status)
{
    Process* current(Process::getCurrentProcess());
    current->exit(status);
}

void* Sched::
map(const void* start, long long length, unsigned int prot, unsigned int flags,
          IPageable* pageable, long long offset)
{
    Process* current(Process::getCurrentProcess());
    return current->map(start, length, prot, flags, pageable, offset);
}

void Sched::
unmap(const void* start, long long length)
{
    Process* current(Process::getCurrentProcess());
    return current->unmap(start, length);
}

ICurrentThread* Sched::
currentThread()
{
    addRef();
    return this;
}

IThread* Sched::
createThread(void* (*start)(void* param), void* param)
{
    Process* current(Process::getCurrentProcess());
    return current->createThread(start, param);
}

void Sched::
yield(void)
{
    Thread::reschedule();
}

IMonitor* Sched::
createMonitor()
{
    return new Monitor;
}

IContext* Sched::
getRoot()
{
    Process* current(Process::getCurrentProcess());
    return current->getRoot();
}

IStream* Sched::
getIn()
{
    Process* current(Process::getCurrentProcess());
    return current->getIn();
}

IStream* Sched::
getOut()
{
    Process* current(Process::getCurrentProcess());
    return current->getOut();
}

IStream* Sched::
getError()
{
    Process* current(Process::getCurrentProcess());
    return current->getError();
}

void* Sched::
setBreak(long long increment)
{
    Process* current(Process::getCurrentProcess());
    return current->setBreak(increment);
}

long long Sched::
getNow()
{
    return DateTime::getNow().getTicks();
}

bool Sched::
trace(bool on)
{
    Process* current(Process::getCurrentProcess());
    return current->trace(on);
}

void Sched::
setStartup(void (*startup)(void* (*start)(void* param), void* param))
{
    Process* current(Process::getCurrentProcess());
    return current->setStartup(startup);
}

void Sched::
setFocus(void* (*focus)(void* param))
{
    Process* current(Process::getCurrentProcess());
    return current->setFocus(focus);
}

//
// IInterface
//

bool Sched::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_ICurrentThread)
    {
        *objectPtr = static_cast<ICurrentThread*>(this);
    }
    else if (riid == IID_ICurrentProcess)
    {
        *objectPtr = static_cast<ICurrentProcess*>(this);
    }
    else if (riid == IID_IRuntime)
    {
        *objectPtr = static_cast<IRuntime*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<ICurrentThread*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Sched::
addRef(void)
{
    return ref.addRef();
}

unsigned int Sched::
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
