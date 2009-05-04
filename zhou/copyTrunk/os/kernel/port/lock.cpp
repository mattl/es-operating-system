/*
 * Copyright 2008 Google Inc.
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
