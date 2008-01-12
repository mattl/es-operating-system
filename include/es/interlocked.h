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

#ifndef NINTENDO_ES_INTERLOCKED_H_INCLUDED
#define NINTENDO_ES_INTERLOCKED_H_INCLUDED

class Interlocked
{
    volatile long count;

public:
    Interlocked(long initial = 0) : count(initial)
    {
    }

    operator long() const
    {
        return count;
    }

#ifdef __i386__
    long increment(void)
    {
        register long eax = 1;

        __asm__ __volatile__ (
            "lock\n"
            "xaddl  %0, %1\n"
            "incl   %0\n"
            : "=a" (eax), "=m" (count) : "0" (eax), "m" (count) );

        return eax;
    }

    long decrement(void)
    {
        register long eax = -1;

        __asm__ __volatile__ (
            "lock\n"
            "xaddl  %0, %1\n"
            "decl   %0\n"
            : "=a" (eax), "=m" (count) : "0" (eax), "m" (count) );

        return eax;
    }

    long exchange(register long value)
    {
        __asm__ __volatile__ (
            "xchgl  %0, %1\n"
            : "=a" (value), "=m" (count) : "0" (value), "m" (count) );

        return value;
    }

    long compareExchange(register long value, register long comparand)
    {
        __asm__ __volatile__ (
            "lock\n"
            "cmpxchgl   %2, %1\n"
            : "=a" (value), "=m" (count)
            : "r" (value), "0" (comparand), "m" (count) );

        return value;
    }

#elif __x86_64__
    long increment(void)
    {
        register long rax = 1;

        __asm__ __volatile__ (
            "lock\n"
            "xadd   %0, %1\n"
            "inc    %0\n"
            : "=a" (rax), "=m" (count) : "0" (rax), "m" (count) );

        return rax;
    }

    long decrement(void)
    {
        register long rax = -1;

        __asm__ __volatile__ (
            "lock\n"
            "xadd   %0, %1\n"
            "dec    %0\n"
            : "=a" (rax), "=m" (count) : "0" (rax), "m" (count) );

        return rax;
    }

    long exchange(register long value)
    {
        __asm__ __volatile__ (
            "xchg   %0, %1\n"
            : "=a" (value), "=m" (count) : "0" (value), "m" (count) );

        return value;
    }

    long compareExchange(register long value, register long comparand)
    {
        __asm__ __volatile__ (
            "lock\n"
            "cmpxchg    %2, %1\n"
            : "=a" (value), "=m" (count)
            : "r" (value), "0" (comparand), "m" (count) );

        return value;
    }
#else   // __x86_64__
    long increment(void);
    long decrement(void);
    long exchange(register long value)
    long compareExchange(register long value, register long comparand)
#endif  // __i386__
};

#endif  // NINTENDO_ES_INTERLOCKED_H_INCLUDED
