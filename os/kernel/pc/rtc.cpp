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
    getTime(epoch);
}

int Rtc::
getCounter(int addr)
{
    Lock::Synchronized method(spinLock);

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
getTime()
{
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

void Rtc::
getTime(long long& ticks)
{
    do {
        ticks = getTime().getTicks();
    } while (ticks != getTime().getTicks());
}

bool Rtc::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IRtc)
    {
        *objectPtr = static_cast<IRtc*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IRtc*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Rtc::
addRef(void)
{
    return ref.addRef();
}

unsigned int Rtc::
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
