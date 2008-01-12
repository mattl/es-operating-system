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
