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

#ifndef NINTENDO_ES_KERNEL_I386_IO_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_IO_H_INCLUDED

#include <es/types.h>

static inline void outpb(int port, u8 value)
{
    __asm__ __volatile__ (
        "outb   %b0, %w1"
        :: "a"(value), "Nd"(port));
}

static inline void outpw(int port, u16 value)
{
    __asm__ __volatile__ (
        "outw   %w0, %w1"
        :: "a"(value), "Nd"(port));
}

static inline void outpl(int port, u32 value)
{
    __asm__ __volatile__ (
        "outl   %0, %w1"
        :: "a"(value), "Nd"(port));
}

static inline u8 inpb(int port)
{
    u8 value;

    __asm__ __volatile__ (
        "inb    %w1, %b0"
        : "=a"(value) : "Nd"(port));
    return value;
}

static inline u16 inpw(int port)
{
    u16 value;

    __asm__ __volatile__ (
        "inw    %w1, %w0"
        : "=a"(value) : "Nd"(port));
    return value;
}

static inline u32 inpl(int port)
{
    u32 value;

    __asm__ __volatile__ (
        "inl    %w1, %0"
        : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outpsb(int port, const void* addr, unsigned long count)
{
    __asm__ __volatile__ (
        "rep\n"
        "outsb"
        : "+S"(addr), "+c"(count) : "d"(port));
}

static inline void outpsw(int port, const void* addr, unsigned long count)
{
    __asm__ __volatile__ (
        "rep\n"
        "outsw"
        : "+S"(addr), "+c"(count) : "d"(port));
}

static inline void outpsl(int port, const void* addr, unsigned long count)
{
    __asm__ __volatile__ (
        "rep\n"
        "outsl"
        : "+S"(addr), "+c"(count) : "d"(port));
}

static inline void inpsb(int port, void* addr, unsigned long count)
{
    __asm__ __volatile__ (
        "rep\n"
        "insb"
        : "+D"(addr), "+c"(count) : "d"(port));
}

static inline void inpsw(int port, void* addr, unsigned long count)
{
    __asm__ __volatile__ (
        "rep\n"
        "insw"
        : "+D"(addr), "+c"(count) : "d"(port));
}

static inline void inpsl(int port, void* addr, unsigned long count)
{
    __asm__ __volatile__ (
        "rep\n"
        "insl"
        : "+D"(addr), "+c"(count) : "d"(port));
}

#endif  // NINTENDO_ES_KERNEL_I386_IO_H_INCLUDED
