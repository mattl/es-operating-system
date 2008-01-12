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

unsigned Thread::
splIdle()
{
    register unsigned eax;

    __asm__ __volatile__ (
        "pushfl\n"
        "popl    %0\n"
        "sti"
        : "=a" (eax));

    return eax;
}

unsigned Thread::
splLo()
{
    register unsigned eax;

    __asm__ __volatile__ (
        "pushfl\n"
        "popl   %0\n"
        "sti"
        : "=a" (eax));

    return eax;
}

unsigned Thread::
splHi()
{
    register unsigned eax;

    __asm__ __volatile__ (
        "pushfl\n"
        "popl   %0\n"
        "cli"
        : "=a" (eax));

    return eax;
}

void Thread::
splX(unsigned x)
{
    __asm__ __volatile__ (
        "pushl   %0\n"
        "popfl"
        :: "r" (x));
}

Thread* Thread::
getCurrentThread()
{
    Core* core = Core::getCurrentCore();
    return core ? core->current : 0;
}

void Thread::
reschedule()
{
    unsigned x = splHi();
    Core* core = Core::getCurrentCore();
    if (!core->yieldable)
    {
        splX(x);
        return;
    }
    Thread* current = core->current;
    if (current)
    {
        if (current->label.set())
        {
            // This thread resumes from here.
            splX(x);
            return;
        }
    }
    core->label.jump();     // Jump to reschedule().
    // NOT REACHED HERE
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
    size = (size + align - 1) & ~(align -1);

    Ureg* ureg(static_cast<Ureg*>(param));
    ureg->esp -= size;
    tcb = reinterpret_cast<void*>(ureg->esp + size);
    return reinterpret_cast<void*>(ureg->esp);
}

void Thread::setArguments(char* arguments)
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
