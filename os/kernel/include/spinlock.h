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

#ifndef NINTENDO_ES_KERNEL_SPINLOCK_H_INCLUDED
#define NINTENDO_ES_KERNEL_SPINLOCK_H_INCLUDED

#ifdef __es__

#include <es.h>
#include <es/interlocked.h>

class Thread;

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

#endif  // __es__

#ifdef __unix__
#include "core.h"
#endif  // __unix__

#endif  // NINTENDO_ES_KERNEL_SPINLOCK_H_INCLUDED
