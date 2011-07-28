/*
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

#include "core.h"
#include "thread.h"

void Thread::Rendezvous::
sleep(Delegate* delegate)
{
    unsigned x = Core::splHi();
    Thread* current = getCurrentThread();
    ASSERT(current);
    for (;;)
    {
        lock();

        ASSERT(current->state == es::Thread::RUNNING);
        if (delegate->invoke(0))
        {
            unlock();
            Core::splX(x);
            return;
        }
#ifdef VERBOSE
        esReport("sleep[%d]: %p\n", Core::getCurrentCore()->getID(), current);
#endif
        current->lock();
        queue.addPrio(current);
        current->state = es::Thread::WAITING;
        current->rendezvous = this;

        // Note the current thread is kept locked in order not to be waken up
        // halfway through the rescheduling.

        unlock();
        reschedule();
    }
    Core::splX(x);
}

// Note wakeup() does not yield CPU immedeately.
void Thread::Rendezvous::
wakeup(Delegate* delegate)
{
    unsigned x = Core::splHi();
    lock();

    if (!delegate || delegate->invoke(1))
    {
        while (!queue.isEmpty())
        {
            Thread* thread = queue.getFirst();
            if (thread->tryLock())
            {
                ASSERT(thread->state != es::Thread::TERMINATED);
                ASSERT(thread->rendezvous == this);
#ifdef VERBOSE
                esReport("wakeup[%d]: %p\n", Core::getCurrentCore()->getID(), thread);
#endif
                queue.remove(thread);
                thread->rendezvous = 0;
                thread->state = es::Thread::RUNNABLE;
                thread->setRun();
                thread->unlock();
            }
            else
            {
                thread->addRef();
                unlock();
                thread->wait();
                thread->release();
                lock();
            }
        }
    }

    unlock();
    Core::splX(x);
}

void Thread::Rendezvous::
remove(Thread* thread)
{
    unsigned x = Core::splHi();
    lock();
    if (thread->rendezvous == this)
    {
        queue.remove(thread);
        thread->rendezvous = 0;
    }
    unlock();
    Core::splX(x);
}

void Thread::Rendezvous::
update(Thread* thread, int effective)
{
    unsigned x = Core::splHi();
    lock();
    if (thread->rendezvous == this)
    {
        queue.remove(thread);
        thread->priority = effective;
        queue.addPrio(thread);
    }
    else
    {
        thread->priority = effective;
    }
    unlock();
    Core::splX(x);
}

int Thread::Rendezvous::
getPriority()
{
    Thread* blocked = queue.getFirst();
    return blocked ? blocked->priority : es::Thread::Lowest;
}
