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

#ifndef NINTENDO_ES_KERNEL_I386_8254_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_8254_H_INCLUDED

// 8254 programmable interval timer

#include <es/dateTime.h>
#include <es/ref.h>
#include <es/base/ICallback.h>
#include <es/device/IBeep.h>
#include "alarm.h"

class Pit : public ICallback, public IBeep
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
    void* queryInterface(const Guid& riid);
    unsigned int addRef();
    unsigned int release();

    friend class Apic;
    friend class Core;
};

#endif // NINTENDO_ES_KERNEL_I386_8254_H_INCLUDED
