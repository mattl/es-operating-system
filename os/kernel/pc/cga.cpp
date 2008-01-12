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

#include <ctype.h>
#include <string.h>
#include <es.h>
#include "io.h"
#include "cga.h"

void Cga::
putc(int c)
{
    unsigned long x;

    switch (c)
    {
    case '\n':
        x = (unsigned long) cga;
        x -= BASE;
        x += 160;
        x -= x % 160;
        x += BASE;
        cga = (char*) x;
        break;
    default:
        *cga = (char) c;
        cga += 2;
        break;
    }

    x = (unsigned long) cga;
    x -= BASE;
    if (160 * 25 <= x)
    {
        memmove((char*) BASE,
                (char*) BASE + 160,
                160 * 24);
        for (cga = (char*) BASE + 160 * 24;
             cga < (char*) BASE + 160 * 25;
             cga += 2)
        {
            *cga = ' ';
        }
        cga = (char*) BASE + 160 * 24;
    }

    x = (unsigned long) cga;
    x -= BASE;
    x >>= 1;
    outpb(CTL_ADDR, CURSOR_ADDR_HIGH);
    outpb(CTL_DATA, x >> 8);
    outpb(CTL_ADDR, CURSOR_ADDR_LOW);
    outpb(CTL_DATA, x);
}

Cga::
Cga()
{
    unsigned long x;

    outpb(CTL_ADDR, CURSOR_ADDR_HIGH);
    x = inpb(CTL_DATA) << 8;
    outpb(CTL_ADDR, CURSOR_ADDR_LOW);
    x |= inpb(CTL_DATA);
    x <<= 1;
    cga = (char*) BASE + x;
}

long long Cga::
getPosition()
{
    return 0;
}

void Cga::
setPosition(long long pos)
{
}

long long Cga::
getSize()
{
    return 0;
}

void Cga::setSize(long long size)
{
}

int Cga::
read(void* dst, int count)
{
    return 0;
}

int Cga::
read(void* dst, int count, long long offset)
{
    return read(dst, count);
}

int Cga::
write(const void* src, int count)
{
    int n;

    const u8* ptr = static_cast<const u8*>(src);
    for (n = 0; n < count; ++n, ++ptr)
    {
        putc(*ptr);
    }
    return n;
}

int Cga::
write(const void* src, int count, long long offset)
{
    return write(src, count);
}

void Cga::
flush()
{
}

bool Cga::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IStream)
    {
        *objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IStream*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Cga::
addRef(void)
{
    return ref.addRef();
}

unsigned int Cga::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
