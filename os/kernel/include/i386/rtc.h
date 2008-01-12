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

#ifndef NINTENDO_ES_KERNEL_I386_RTC_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_RTC_H_INCLUDED

#include <es/dateTime.h>
#include <es/ref.h>
#include <es/device/IRtc.h>

class Rtc : public IRtc
{
    friend DateTime DateTime::getNow();

    enum
    {
        PORT_ADDR = 0x70,
        PORT_DATA = 0x71,

        SECONDS = 0x00,
        MINUTES = 0x02,
        HOURS = 0x04,
        DAY = 0x07,
        MONTH = 0x08,
        YEAR = 0x09,
        PORT_A = 0x0A,      // usually 0x26
        PORT_B = 0x0B,      // usually 0x02
        YEAR_FTD = 0x32
    };

    Ref                 ref;

    static SpinLock     spinLock;
    static long long    epoch;

    static int getCounter(int addr);
    static void setCounter(int addr, int count);
    static DateTime getTime();

public:
    Rtc();

    // IRtc
    void getTime(long long& time);
    void setTime(long long time);

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);
};

#endif // NINTENDO_ES_KERNEL_I386_RTC_H_INCLUDED
