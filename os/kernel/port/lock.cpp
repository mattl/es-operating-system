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

namespace
{
    long long int rdtsc()
    {
         long long tsc;

         __asm__ volatile ("rdtsc" : "=A" (tsc));
         return tsc;
    }
}

Lock::
Lock() :
    spin(0)
{
}

Lock::
~Lock()
{
}

bool Lock::
isLocked()
{
    return (spin != 0) ? true : false;
}

void Lock::
wait()
{
    long long count = rdtsc();

    while (spin)
    {
        ASSERT(rdtsc() - count < 10000000000LL);
#if defined(__i386__) || defined(__x86_64__)
        // Use the pause instruction for Hyper-Threading
        __asm__ __volatile__ ("pause\n");
#endif
    }
}

bool Lock::
tryLock()
{
    return spin.exchange(1) ? false : true;
}

void Lock::
lock()
{
    ASSERT(1 < Sched::numCores || !spin);
    do
    {
        wait();
    }
    while (!tryLock());
}

void Lock::
unlock()
{
    ASSERT(isLocked());
    spin.exchange(0);
}

Lock::
Synchronized::Synchronized(Lock& spinLock) :
    spinLock(spinLock)
{
    x = Core::splHi();
    spinLock.lock();
}

Lock::
Synchronized::~Synchronized()
{
    spinLock.unlock();
    Core::splX(x);
}
