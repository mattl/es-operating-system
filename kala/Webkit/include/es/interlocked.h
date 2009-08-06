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

#ifndef NINTENDO_ES_INTERLOCKED_H_INCLUDED
#define NINTENDO_ES_INTERLOCKED_H_INCLUDED

/**
 * This class provides atomic operations for variables.
 */
class Interlocked
{
    volatile long count;

public:
    /**
     * Creates a new object which has the specified count.
     */
    Interlocked(long initial = 0) : count(initial)
    {
    }

    /**
     * Conversion operator to long.
     */
    operator long() const
    {
        return count;
    }

#ifdef __i386__
    /**
     * Increments the count of this object as an atomic operation.
     * @return the count after the operation.
     */
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

    /**
     * Decrements the count of this object as an atomic operation.
     * @return the count after the operation.
     */
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

    /**
     * Sets the specified value to the count of this object as an atomic operation.
     * @param value the value to be set..
     * @return the count before the operation.
     */
    long exchange(register long value)
    {
        __asm__ __volatile__ (
            "xchgl  %0, %1\n"
            : "=a" (value), "=m" (count) : "0" (value), "m" (count) );

        return value;
    }

    /**
     * Compares the count of this object and the specified value for equality,
     * and if they are equal, sets the specified value to the count.
     * @param value the value to be set to the count.
     * @param comparand the value to be compared with the count.
     * @return the count before the operation.
     */
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
    /**
     * Increments the count of this object as an atomic operation.
     * @return the count after the operation.
     */
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

    /**
     * Decrements the count of this object as an atomic operation.
     * @return the count after the operation.
     */
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

    /**
     * Sets the specified value to the count of this object as an atomic operation.
     * @param value the value to be set..
     * @return the count before the operation.
     */
    long exchange(register long value)
    {
        __asm__ __volatile__ (
            "xchg   %0, %1\n"
            : "=a" (value), "=m" (count) : "0" (value), "m" (count) );

        return value;
    }

    /**
     * Compares the count of this object and the specified value for equality,
     * and if they are equal, sets the specified value to the count.
     * @param value the value to be set to the count.
     * @param comparand the value to be compared with the count.
     * @return the count before the operation.
     */
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
    /**
     * Increments the count of this object as an atomic operation.
     * @return the count after the operation.
     */
    long increment(void);
    /**
     * Decrements the count of this object as an atomic operation.
     * @return the count after the operation.
     */
    long decrement(void);
    /**
     * Sets the specified value to the count of this object as an atomic operation.
     * @param value the value to be set..
     * @return the count before the operation.
     */
    long exchange(register long value)
    /**
     * Compares the count of this object and the specified value for equality,
     * and if they are equal, sets the specified value to the count.
     * @param value the value to be set to the count.
     * @param comparand the value to be compared with the count.
     * @return the count before the operation.
     */
    long compareExchange(register long value, register long comparand)
#endif  // __i386__
};

#endif  // NINTENDO_ES_INTERLOCKED_H_INCLUDED
