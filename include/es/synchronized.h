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

#ifndef NINTENDO_ES_SYNCHRONIZED_H_INCLUDED
#define NINTENDO_ES_SYNCHRONIZED_H_INCLUDED

template<class I>
class Synchronized
{
    I& lock;

    Synchronized& operator=(const Synchronized&);

public:
    Synchronized(I& lock) : lock(lock)
    {
        lock->lock();
    }
    ~Synchronized()
    {
        lock->unlock();
    }
};

#endif  // NINTENDO_ES_SYNCHRONIZED_H_INCLUDED
