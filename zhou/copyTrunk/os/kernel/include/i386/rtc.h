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

#ifndef NINTENDO_ES_KERNEL_I386_RTC_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_RTC_H_INCLUDED

#include <es/dateTime.h>
#include <es/ref.h>
#include <es/device/IRtc.h>



class Rtc : public es::Rtc
{
    friend DateTime DateTime::getNow();

    Ref                 ref;

    static Lock         spinLock;
    static long long    epoch;

    static int getCounter(int addr);
    static void setCounter(int addr, int count);
    static DateTime getDateTime();

public:

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

    Rtc();

    // IRtc
    long long getTime();
    void setTime(long long time);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    static u8 cmosRead(u8 offset);
    static void cmosWrite(u8 offset, u8 val);
};

#endif // NINTENDO_ES_KERNEL_I386_RTC_H_INCLUDED
