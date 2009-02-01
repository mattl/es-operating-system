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

    if (current->state == es::Thread::RUNNING && !core->sched->runQueueHint)
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
    argv[n] = 0;
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
