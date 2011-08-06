/*
 * Copyright 2011 Esrille Inc.
 * Copyright 2008, 2009 Google Inc.
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

#include <ctype.h>
#include <string.h>
#include <es.h>
#include "io.h"
#include "cga.h"

void Cga::
putChar(int c)
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
    case 0x08: // backspace
        x = (unsigned long) cga;
        x -= BASE;
        x -= x % 160;
        x += BASE;

        if ((unsigned long) (cga - 2) < x)
        {
            cga = (char*) x; // head of line.
        }
        else
        {
            cga -= 2;
        }
        *cga = ' ';
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
        putChar(*ptr);
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

Object* Cga::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Stream::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;

}

unsigned int Cga::
addRef()
{
    return ref.addRef();
}

unsigned int Cga::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
