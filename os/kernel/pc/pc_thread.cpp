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

#include <string.h>
#include <es.h>
#include "thread.h"
#include "core.h"

Thread* Thread::
getCurrentThread()
{
    if (Sched::numCores == 0)
    {
        return 0;
    }

    unsigned x = Core::splHi();
    Core* core = Core::getCurrentCore();
    Thread* current = core ? core->current : 0;
    Core::splX(x);
    return current;
}

void Thread::
reschedule()
{
    unsigned x = Core::splHi();
    Core* core = Core::getCurrentCore();
    if (core->freeze)
    {
        Core::splX(x);
        return;
    }
    Thread* current = core->current;

    if (!current)
    {
        core->label.jump();     // Jump to Core::reschedule().
        // NOT REACHED HERE
    }

    if (current->state == IThread::RUNNING && !core->sched->runQueueHint)
    {
        current->tryLock();
        current->unlock();
        Core::splX(x);
        return;
    }

    ASSERT(current->core == core);

    if (current->label.set() == 0)
    {
        core->label.jump();     // Jump to Core::reschedule().
        // NOT REACHED HERE
    }

    // This thread resumes from here.
    Core::splX(x);
}

void Thread::
push(unsigned arg)
{
    Ureg* ureg(static_cast<Ureg*>(param));
    unsigned* frame = reinterpret_cast<unsigned*>(ureg->esp);
    *--frame = arg;
    ureg->esp = reinterpret_cast<unsigned>(frame);
}

void Thread::
entry(unsigned long start)
{
    ASSERT(state == NEW);

    Ureg* ureg(static_cast<Ureg*>(param));
    ureg->eip = start;
}

void* Thread::
tls(unsigned size, unsigned align)
{
    unsigned x = (size + sizeof(void*) + align - 1) & ~(align -1);

    Ureg* ureg(static_cast<Ureg*>(param));
    ureg->esp -= x;
    tcb = reinterpret_cast<void*>(ureg->esp + size);
    *(void**) tcb = tcb;
    return reinterpret_cast<void*>(ureg->esp);
}

void Thread::
setArguments(char* arguments)
{
    ASSERT(arguments[-1] == '\0');
    Ureg* ureg(static_cast<Ureg*>(param));
    unsigned* frame = reinterpret_cast<unsigned*>(ureg->esp);

    // Count the number of arguments.   XXX Add double quote and back-slash handling
    int argc(0);
    char* p(arguments);
    while (*p)
    {
        if (*p != ' ')
        {
            ++argc;
            while (*p && *p != ' ')
            {
                ++p;
            }
        }
        while (*p == ' ')
        {
            ++p;
        }
    }

    // Copy-out arguments.
    int n(argc);
    frame -= n + 1;
    char** argv = (char**) frame;
    char* a = (char*) frame;
    *--a = '\0';
    argv[n] = a;
    --p;
    while (*p)
    {
        while (*p == ' ')
        {
            --p;
        }
        *--a = '\0';
        while (*p && *p != ' ')
        {
            *--a = *p--;
        }
        argv[--n] = a;
    }

    // Push argc and argv.
    frame = (unsigned*) (((unsigned long) a) & ~(sizeof(unsigned long) - 1));
    *--frame = (unsigned) argv;
    *--frame = (unsigned) argc;
    ureg->esp = reinterpret_cast<unsigned>(frame);
}
