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

#ifndef NINTENDO_ES_KERNEL_SPINLOCK_H_INCLUDED
#define NINTENDO_ES_KERNEL_SPINLOCK_H_INCLUDED

#ifdef __es__

#include <es.h>
#include <es/interlocked.h>

class Thread;

/** A non-reentrant spinlock.
 */
class Lock
{
    Interlocked spin;

public:
    Lock();
    ~Lock();
    bool isLocked();
    void wait();
    bool tryLock();
    void lock();
    void unlock();

    class Synchronized
    {
        Lock& spinLock;
        unsigned x;
        Synchronized& operator=(const Synchronized&);
    public:
        Synchronized(Lock& spinLock);
        ~Synchronized();
    };
};

/** A reentrant spinlock.
 */
class SpinLock
{
    Interlocked spin;
    Thread*     owner;
    unsigned    count;

public:
    SpinLock();
    bool isLocked();
    void wait();
    bool tryLock();
    void lock();
    void unlock();

    class Synchronized
    {
        SpinLock&   spinLock;
        unsigned    x;
        Synchronized& operator=(const Synchronized&);
    public:
        Synchronized(SpinLock& spinLock);
        ~Synchronized();
    };
};

#else   // !__es__

#include "core.h"

#endif  // !__es__


#endif  // NINTENDO_ES_KERNEL_SPINLOCK_H_INCLUDED
