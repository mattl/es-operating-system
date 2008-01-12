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

#include "thread.h"

void Thread::Rendezvous::
sleep(Delegate* delegate)
{
    for (;;)
    {
        unsigned x = splHi();
        Thread* current = getCurrentThread();
        ASSERT(current);
        current->lock();
        ASSERT(current->state == IThread::RUNNING);
        lock();

        // Insert current to queue so that getPriority() can return the correct
        // priority for the delegate function.
        queue.addPrio(current);

        if (delegate->invoke(0))
        {
            queue.remove(current);
            current->unlock();
            unlock();
            splX(x);
            return;
        }

        current->state = IThread::WAITING;
        current->rendezvous = this;

        current->unlock();
        unlock();
        splX(x);

        reschedule();
    }
}

// Note wakeup() does not yield CPU immedeately.
void Thread::Rendezvous::
wakeup(Delegate* delegate)
{
    unsigned x = splHi();
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
    splX(x);
}

void Thread::Rendezvous::
remove(Thread* thread)
{
    unsigned x = splHi();
    lock();
    if (thread->rendezvous == this)
    {
        queue.remove(thread);
        thread->rendezvous = 0;
    }
    unlock();
    splX(x);
}

void Thread::Rendezvous::
update(Thread* thread, int effective)
{
    unsigned x = splHi();
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
    splX(x);
}

int Thread::Rendezvous::
getPriority()
{
    Thread* blocked = queue.getFirst();
    if (blocked)
    {
        return blocked->priority;
    }
    return IThread::Lowest;
}
