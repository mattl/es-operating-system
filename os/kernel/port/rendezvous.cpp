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

        ASSERT(current->state == IThread::RUNNING);
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
        current->state = IThread::WAITING;
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
                ASSERT(thread->state != IThread::TERMINATED);
                ASSERT(thread->rendezvous == this);
#ifdef VERBOSE
                esReport("wakeup[%d]: %p\n", Core::getCurrentCore()->getID(), thread);
#endif
                queue.remove(thread);
                thread->rendezvous = 0;
                thread->state = IThread::RUNNABLE;
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
    return blocked ? blocked->priority : IThread::Lowest;
}
