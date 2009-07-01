/*
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

#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/usage.h>
#include <es/handle.h>
#include "io.h"
#include "8237a.h"

const u8 Dmac::pageOffset[4] = { 7, 3, 1, 2 };

Dmac::
Dmac(u8 base, u8 page, int shift) :
    base(base),
    page(page),
    shift(shift)
{
    for (u8 n = 0; n < 4; ++n)
    {
        chan[n].dmac = this;
        chan[n].chan = n;
        chan[n].stop();
    }

    if (shift)  // master?
    {
        // Set channel 0 to the cascade mode.
        setup(0, 0, 0, 0xc0);
        // Enable slave controller
        chan[0].start();
    }
}

void Dmac::
setup(u8 chan, u32 buffer, int len, u8 mode)
{
    ASSERT(buffer < 16 * 1024 * 1024);
    outpb(base + (MODE << shift), mode | chan);
    outpb(page + pageOffset[chan], buffer >> 16);
    outpb(base + (CLEAR_BYTE_POINTER << shift), 0);
    u8 offset = (((chan << 1) + ADDR) << shift);
    outpb(base + offset, buffer >> shift);
    outpb(base + offset, buffer >> (8 + shift));
    offset = (((chan << 1) + COUNT) << shift);
    outpb(base + offset, (len >> shift) - 1);
    outpb(base + offset, ((len >> shift) - 1) >> 8);
}

void Dmac::
Chan::setup(const void* buffer, int len, u8 mode)
{
    Lock::Synchronized method(dmac->spinLock);

    mode &= es::Dmac::READ | es::Dmac::WRITE | es::Dmac::AUTO_INITIALIZE;
    dmac->setup(chan, ((u32) buffer) & ~0xc0000000, len, mode | 0x40);  // single mode
}

void Dmac::
Chan::start()
{
    Lock::Synchronized method(dmac->spinLock);

    outpb(dmac->base + (SINGLE_MASK << dmac->shift), chan);
}

void Dmac::
Chan::stop()
{
    Lock::Synchronized method(dmac->spinLock);

    outpb(dmac->base + (SINGLE_MASK << dmac->shift), 0x04 | chan);
}

bool Dmac::
Chan::isDone()
{
    Lock::Synchronized method(dmac->spinLock);

    return inpb(dmac->base + (COMMAND << dmac->shift)) & (1 << chan);
}

int Dmac::
Chan::getCount()
{
    Lock::Synchronized method(dmac->spinLock);

    outpb(dmac->base + (CLEAR_BYTE_POINTER << dmac->shift), 0);
    int count = inpb(dmac->base + (((chan << 1) + COUNT) << dmac->shift));
    count |= inpb(dmac->base + (((chan << 1) + COUNT) << dmac->shift)) << 8;
    return (count << dmac->shift) + 1;
}

Object* Dmac::
Chan::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Dmac::iid()) == 0)
    {
        objectPtr = static_cast<es::Dmac*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Dmac*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Dmac::
Chan::addRef()
{
    return dmac->ref.addRef();
}

unsigned int Dmac::
Chan::release()
{
    unsigned int count = dmac->ref.release();
    if (count == 0)
    {
        delete dmac;
        return 0;
    }
    return count;
}
