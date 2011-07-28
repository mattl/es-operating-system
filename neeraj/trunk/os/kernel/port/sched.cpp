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
    thread->state = es::Thread::RUNNABLE;
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
    ASSERT(thread->state == es::Thread::RUNNABLE);
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
        priority = es::Thread::Highest + 1 - ffs(runQueueBits);
        if (priority <= es::Thread::Highest && es::Thread::Lowest <= priority)
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

    ASSERT(next->state == es::Thread::RUNNABLE);
    ASSERT(next->priority == priority);
    queue->remove(next);
    ASSERT(!queue->contains(next));
    if (queue->isEmpty())
    {
        runQueueBits &= ~(0x80000000u >> priority);
    }
    next->state = es::Thread::RUNNING;
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
exit(void* val)
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
map(void* start, long long length, unsigned int prot, unsigned int flags,
    es::Pageable* pageable, long long offset)
{
    Process* current(Process::getCurrentProcess());
    return current->map(start, length, prot, flags, pageable, offset);
}

void Sched::
unmap(void* start, long long length)
{
    Process* current(Process::getCurrentProcess());
    return current->unmap(start, length);
}

es::CurrentThread* Sched::
currentThread()
{
    addRef();
    return this;
}

es::Thread* Sched::
// createThread(void* (*start)(void* param), void* param) // [check]
createThread(void* start, void* param)
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

es::Monitor* Sched::
createMonitor()
{
    return new Monitor;
}

es::Context* Sched::
getRoot()
{
    Process* current(Process::getCurrentProcess());
    return current->getRoot();
}

es::Stream* Sched::
getInput()
{
    Process* current(Process::getCurrentProcess());
    return current->getInput();
}

es::Stream* Sched::
getOutput()
{
    Process* current(Process::getCurrentProcess());
    return current->getOutput();
}

es::Stream* Sched::
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
setCurrent(es::Context* context)
{
    Process* current(Process::getCurrentProcess());
    return current->setCurrent(context);
}

es::Context* Sched::
getCurrent()
{
    Process* current(Process::getCurrentProcess());
    return current->getCurrent();
}

void Sched::
setStartup(void* startup) // [check] setStartup(void (*startup)(void* (*start)(void* param), void* param))
{
    Process* current(Process::getCurrentProcess());
    return current->setStartup(startup);
}

void Sched::

setFocus(void* focus) // [check] setFocus(void* (*focus)(void* param))
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

Object* Sched::
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
    else if (strcmp(riid, es::Runtime::iid()) == 0)
    {
        objectPtr = static_cast<es::Runtime*>(this);
    }
    else if (strcmp(riid, es::Callback::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
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

unsigned int Sched::
addRef()
{
    return ref.addRef();
}

unsigned int Sched::
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
