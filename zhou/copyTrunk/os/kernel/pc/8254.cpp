/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

Object* Pit::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Callback::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else if (strcmp(riid, es::Beep::iid()) == 0)
    {
        objectPtr = static_cast<es::Beep*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
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
