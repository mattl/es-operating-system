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

// 8254 programmable interval timer

#include <es.h>
#include "cpu.h"
#include "io.h"
#include "8254.h"
#include "core.h"
#include "thread.h"

long long Pit::tick;

Pit::
Pit(unsigned hz) : hz(hz), beeper(0)
{
    // Set up counter 0
    u16 count = (143181800 / hz / 12 + 5) / 10;
    outpb(PORT_CONTROL, COUNTER_0 | COUNTER_HI | COUNTER_LO | MODE_SQUARE);
    outpb(PORT_COUNTER_0, count);
    outpb(PORT_COUNTER_0, count >> 8);
    Core::registerExceptionHandler(32 + 0, this);   // XXX startup irq

    // Set up counter 2 (beep)
    setFrequency(750);
    setDuration(125);
}

//
// ICallback
//

int Pit::
invoke(int)
{
    tick += 10000000LL / hz;
    Alarm::invoke();

    if (0 < beeper)
    {
        beeper -= 10000000LL / hz;
        if (beeper <= 0)
        {
            beeper = 0;
            outpb(SYSTEM_PORT, inpb(SYSTEM_PORT) & ~0x03);
        }
    }

    return 0;
}

//
// IBeep
//

void Pit::
setDuration(unsigned msec)
{
    duration = msec;
}

void Pit::
setFrequency(unsigned hz)
{
    freq = hz;
    unsigned count = (143181800 / hz / 12 + 5) / 10;

    unsigned x = Core::splHi();
    outpb(PORT_CONTROL, COUNTER_2 | COUNTER_HI | COUNTER_LO | MODE_SQUARE);
    outpb(PORT_COUNTER_2, count);
    outpb(PORT_COUNTER_2, count >> 8);
    Core::splX(x);
}

unsigned Pit::
getDuration()
{
    return duration;
}

unsigned Pit::
getFrequency()
{
    return freq;
}

void Pit::
beep()
{
    unsigned x = Core::splHi();
    beeper = duration * 10000LL;
    outpb(SYSTEM_PORT, inpb(SYSTEM_PORT) | 0x03);
    Core::splX(x);
}

//
// IInterface
//

bool Pit::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_ICallback)
    {
        *objectPtr = static_cast<ICallback*>(this);
    }
    else if (riid == IID_IBeep)
    {
        *objectPtr = static_cast<IBeep*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<ICallback*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Pit::
addRef(void)
{
    return ref.addRef();
}

unsigned int Pit::
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
