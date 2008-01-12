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

#include "core.h"
#include "thread.h"

namespace
{
    long long int rdtsc()
    {
         long long tsc;

         __asm__ volatile ("rdtsc" : "=A" (tsc));
         return tsc;
    }
}

SpinLock::
SpinLock() :
    spin(0),
    owner(0),
    count(0)
{
}

bool SpinLock::
isLocked()
{
    if (!spin)
    {
        return false;
    }
    if (owner == Thread::getCurrentThread())
    {
        return false;
    }
    return true;
}

void SpinLock::
wait()
{
    Thread* current = Thread::getCurrentThread();
    long long count = rdtsc();

    while (spin && owner != current)
    {
        ASSERT(rdtsc() - count < 10000000000LL);
#if defined(__i386__) || defined(__x86_64__)
        // Use the pause instruction for Hyper-Threading
        __asm__ __volatile__ ("pause\n");
#endif
    }
}

bool SpinLock::
tryLock()
{
    Thread* current = Thread::getCurrentThread();
    ASSERT(Sched::numCores == 0 || current);
    if (!spin.exchange(1))
    {
        ASSERT(count == 0);
        count = 1;
        owner = current;
        return true;
    }
    if (owner == current)
    {
        ++count;
        return true;
    }
    return false;
}

void SpinLock::
lock()
{
    ASSERT(1 < Sched::numCores || !spin || owner == Thread::getCurrentThread());
    do
    {
        wait();
    }
    while (!tryLock());
    ASSERT(0 < count);
}

void SpinLock::
unlock()
{
    ASSERT(owner == Thread::getCurrentThread());
    ASSERT(0 < count);
    if (--count == 0)
    {
        owner = 0;
        spin.exchange(0);
    }
}

SpinLock::Synchronized::
Synchronized(SpinLock& spinLock) :
    spinLock(spinLock)
{
    x = Core::splHi();
    spinLock.lock();
}

SpinLock::Synchronized::
~Synchronized()
{
    spinLock.unlock();
    Core::splX(x);
}
