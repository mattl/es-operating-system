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

// Real Time Clock (MC146818A)

#include <es.h>
#include "alarm.h"
#include "io.h"
#include "rtc.h"

Lock Rtc::spinLock;
long long Rtc::epoch;

Rtc::
Rtc() :
    ref(1)
{
    epoch = getTime();
}

int Rtc::
getCounter(int addr)
{
    outpb(PORT_ADDR, addr);
    u8 bcd = inpb(PORT_DATA);
    return (bcd & 0xf) + 10 * (bcd >> 4);
}

void Rtc::
setCounter(int addr, int count)
{
    Lock::Synchronized method(spinLock);

    outpb(PORT_ADDR, addr);
    outpb(PORT_DATA, (count % 10) | ((count / 10 % 10) << 4));
}

DateTime Rtc::
getDateTime()
{
    Lock::Synchronized method(spinLock);

    do
    {
        outpb(Rtc::PORT_ADDR, Rtc::PORT_A);
    }
    while (inpb(Rtc::PORT_DATA) & 0x80);    // wait UIP

    int second = getCounter(SECONDS);
    int minute = getCounter(MINUTES);
    int hour = getCounter(HOURS);
    int day = getCounter(DAY);
    int month = getCounter(MONTH);
    int year = getCounter(YEAR) + getCounter(YEAR_FTD) * 100;
    return DateTime(year, month, day, hour, minute, second);
}

void Rtc::
setTime(long long ticks)
{
    Lock::Synchronized method(spinLock);

    DateTime date(ticks);

    // Stop RTC
    outpb(PORT_ADDR, PORT_A);
    u8 a = inpb(PORT_DATA);
    a |= 0x70;
    outpb(PORT_DATA, a);

    setCounter(SECONDS, date.getSecond());
    setCounter(MINUTES, date.getMinute());
    setCounter(HOURS, date.getHour());
    setCounter(DAY, date.getDay());
    setCounter(MONTH, date.getMonth());
    int year = date.getYear();
    setCounter(YEAR, year % 100);
    setCounter(YEAR_FTD, year / 100);

    // Restart RTC
    outpb(PORT_ADDR, PORT_A);
    a &= ~0x70;
    a |= 0x20;
    outpb(PORT_DATA, a);

    epoch += ticks - DateTime::getNow().getTicks();
}

long long Rtc::
getTime()
{
    long long ticks;
    do {
        ticks = getDateTime().getTicks();
    } while (ticks != getDateTime().getTicks());

    return ticks;
}

Object* Rtc::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Rtc::iid()) == 0)
    {
        objectPtr = static_cast<es::Rtc*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Rtc*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Rtc::
addRef()
{
    return ref.addRef();
}

unsigned int Rtc::
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

u8 Rtc::
cmosRead(u8 offset)
{
    ASSERT(0x0e <= offset);
    outpb(PORT_ADDR, offset);
    return inpb(PORT_DATA);
}

void Rtc::
cmosWrite(u8 offset, u8 val)
{
    ASSERT(0x0e <= offset);
    outpb(PORT_ADDR, offset);
    outpb(PORT_DATA, val);
}
