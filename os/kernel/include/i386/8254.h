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

#ifndef NINTENDO_ES_KERNEL_I386_8254_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_8254_H_INCLUDED

// 8254 programmable interval timer

#include <es/dateTime.h>
#include <es/ref.h>
#include <es/base/ICallback.h>
#include <es/device/IBeep.h>
#include "alarm.h"

class Pit : public es::Callback, public es::Beep
{
    friend DateTime DateTime::getNow();

    Ref         ref;
    Lock        spinLock;
    unsigned    hz;         // for counter 0

    Alarm       alarm;
    unsigned    freq;       // for counter 2 (beep)
    unsigned    duration;
    long long   beeper;

    static long long tick;

public:
    explicit Pit(unsigned hz);

    static const int PORT_COUNTER_0 = 0x40;
    static const int PORT_COUNTER_1 = 0x41;
    static const int PORT_COUNTER_2 = 0x42;
    static const int PORT_CONTROL = 0x43;

    static const u8 COUNTER_0 = 0x00;
    static const u8 COUNTER_1 = 0x40;
    static const u8 COUNTER_2 = 0x80;
    static const u8 COUNTER_HI = 0x20;
    static const u8 COUNTER_LO = 0x10;
    static const u8 COUNTER_LATCH = 0x00;
    static const u8 MODE_SQUARE = 6;
    static const u8 SYSTEM_PORT = 0x61;

    // ICallback
    int invoke(int);

    // IBeep
    void setFrequency(unsigned freq);
    void setDuration(unsigned msec);
    unsigned getFrequency();
    unsigned getDuration();
    void beep();

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    friend class Apic;
    friend class Core;
};

#endif // NINTENDO_ES_KERNEL_I386_8254_H_INCLUDED
