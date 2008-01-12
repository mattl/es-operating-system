/*
 * Copyright (c) 2006, 2007
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
    if (0 < hz)
    {
        u16 count = (143181800 / hz / 12 + 5) / 10;
        outpb(PORT_CONTROL, COUNTER_0 | COUNTER_HI | COUNTER_LO | MODE_SQUARE);
        outpb(PORT_COUNTER_0, count);
        outpb(PORT_COUNTER_0, count >> 8);
        Core::registerInterruptHandler(0, this);
    }

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
    if (hz)
    {
        tick += 10000000LL / hz;
        Alarm::invoke();
    }

    if (beeper &&  (0 < tick - beeper || !hz))
    {
        Lock::Synchronized method(spinLock);

        beeper = 0;
        outpb(SYSTEM_PORT, inpb(SYSTEM_PORT) & ~0x03);
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
    Lock::Synchronized method(spinLock);

    freq = hz;
    unsigned count = (143181800 / hz / 12 + 5) / 10;
    outpb(PORT_CONTROL, COUNTER_2 | COUNTER_HI | COUNTER_LO | MODE_SQUARE);
    outpb(PORT_COUNTER_2, count);
    outpb(PORT_COUNTER_2, count >> 8);
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
    Lock::Synchronized method(spinLock);

    alarm.setInterval(duration * 10000LL);
    alarm.setCallback(this);
    alarm.setEnabled(true);

    beeper = tick + duration * 10000LL;
    outpb(SYSTEM_PORT, inpb(SYSTEM_PORT) | 0x03);
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
addRef()
{
    return ref.addRef();
}

unsigned int Pit::
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
