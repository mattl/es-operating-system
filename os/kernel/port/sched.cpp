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

#include <string.h> // ffs()
#include "apic.h"
#include "core.h"
#include "thread.h"
#include "process.h"

Ref Sched::numCores(0);

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
    thread->state = IThread::RUNNABLE;
    Thread::Queue* queue = &runQueue[thread->priority];
    queue->addLast(thread);
    runQueueBits |= 0x80000000u >> thread->priority;
    runQueueHint = true;   // Hint to scheduler to check run queue
    unlock();
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
            unsigned x = Core::splIdle();
            while (runQueueBits == 0)
            {
#ifdef __i386__
                __asm__ __volatile__ ("hlt\n");
#endif
            }
            Core::splX(x);
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

    ASSERT(next->state == IThread::RUNNABLE);
    ASSERT(next->priority == priority);
    queue->remove(next);
    ASSERT(!queue->contains(next));
    if (queue->isEmpty())
    {
        runQueueBits &= ~(0x80000000u >> priority);
    }
    next->state = IThread::RUNNING;
    next->core = Core::getCurrentCore();

    unlock();
    ASSERT(next->checkStack());
    next->unlock();     // XXX check if we can unlock next now

    return next;
}

//
//  ICurrentThread
//

void Sched::
exit(const void* val)
{
    Thread* current(Thread::getCurrentThread());
    current->exit(const_cast<void*>(val));
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
// createThread(void* (*start)(void* param), void* param) // [check]
createThread(const void* start, const void* param)
{
    typedef void* (*Start)(void* param); // [check]

    Process* current(Process::getCurrentProcess());
    return current->createThread(reinterpret_cast<Start>(start), const_cast<void*>(param)); // [check]
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
getInput()
{
    Process* current(Process::getCurrentProcess());
    return current->getInput();
}

IStream* Sched::
getOutput()
{
    Process* current(Process::getCurrentProcess());
    return current->getOutput();
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
setCurrent(IContext* context)
{
    Process* current(Process::getCurrentProcess());
    return current->setCurrent(context);
}

IContext* Sched::
getCurrent()
{
    Process* current(Process::getCurrentProcess());
    return current->getCurrent();
}

void Sched::
setStartup(const void* startup) // [check] setStartup(void (*startup)(void* (*start)(void* param), void* param))
{
    Process* current(Process::getCurrentProcess());
    return current->setStartup(startup);
}

void Sched::

setFocus(const void* focus) // [check] setFocus(void* (*focus)(void* param))
{
    Process* current(Process::getCurrentProcess());
    return current->setFocus(focus); // [check]
}

//
// ICallback
//

int Sched::
invoke(int result)
{
    // Process IPIs
    int vec = 32 + result;
}

//
// IInterface
//

void* Sched::
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
    else if (riid == IRuntime::iid())
    {
        objectPtr = static_cast<IRuntime*>(this);
    }
    else if (riid == ICallback::iid())
    {
        objectPtr = static_cast<ICallback*>(this);
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
