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

#ifndef NINTENDO_ES_REF_H_INCLUDED
#define NINTENDO_ES_REF_H_INCLUDED

/**
 * This class represents a reference count.
 */
class Ref
{
    volatile long count;
public:
    /**
     * Constructs a reference count.
     * @param initial the initial value of this reference count.
     */
    Ref(long initial = 1) : count(initial)
    {
    }

    /**
     * A conversion operator.
     */
    operator long() const
    {
        return count;
    }

#ifdef __i386__
    /**
     * Increments this reference count.
     * @return the new reference count.
     */
    long addRef(void)
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
     * Decrements this reference count.
     * @return the new reference count.
     */
    long release(void)
    {
        register long eax = -1;

        __asm__ __volatile__ (
            "lock\n"
            "xaddl  %0, %1\n"
            "decl   %0\n"
            : "=a" (eax), "=m" (count) : "0" (eax), "m" (count) );

        return eax;
    }
#elif __x86_64__
    /**
     * Increments this reference count.
     * @return the new reference count.
     */
    long addRef(void)
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
     * Decrements this reference count.
     * @return the new reference count.
     */
    long release(void)
    {
        register long rax = -1;

        __asm__ __volatile__ (
            "lock\n"
            "xadd   %0, %1\n"
            "dec    %0\n"
            : "=a" (rax), "=m" (count) : "0" (rax), "m" (count) );

        return rax;
    }
#else   // __i386__
    /**
     * Increments this reference count.
     * @return the new reference count.
     */
    long addRef(void);
    /**
     * Decrements this reference count.
     * @return the new reference count.
     */
    long release(void);
#endif  // __i386__
};

#endif  // NINTENDO_ES_REF_H_INCLUDED
