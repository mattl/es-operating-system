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

#ifndef NINTENDO_ES_KERNEL_ALARM_H_INCLUDED
#define NINTENDO_ES_KERNEL_ALARM_H_INCLUDED

#include <es/list.h>
#include <es/ref.h>
#include <es/base/IAlarm.h>
#include "spinlock.h"

class Alarm : public IAlarm
{
    enum
    {
        Enabled = 0x01,
        Periodic = 0x02
    };

    Link<Alarm>     link;
    class Queue
    {
        List<Alarm, &Alarm::link> queue;
    public:
        void process(long long ticks);
        void add(Alarm* alarm);
        void remove(Alarm* alarm);
        long long getDelta(long long now);
        bool check();
    };

    Ref             ref;
    ICallback*      callback;
    unsigned        flags;
    long long       interval;
    long long       start;
    long long       nextTick;
    Queue*          current;

    static Lock     spinLock;
    static Queue    queues[2];  // 0: abs, 1: relative

    bool getFlag(unsigned flag)
    {
        return (flags & flag) ? true : false;
    }

    void setFlag(unsigned flag)
    {
        flags |= flag;
    }

    void clearFlag(unsigned flag)
    {
        flags &= ~flag;
    }

    void cancel();

    static void set(Alarm* alarm);
    static void update();

public:
    Alarm();
    ~Alarm();

    static void invoke();   // XXX Should have a separate interface

    bool isExpired(long long now);

    static bool check();

    // IAlarm
    void getInterval(long long& interval);
    void getStartTime(long long& time);
    bool isEnabled();
    bool isPeriodic();
    void setCallback(ICallback* callback);
    void setEnabled(bool enabled);
    void setPeriodic(bool periodic);
    void setInterval(long long interval);
    void setStartTime(long long time);

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);
};

#endif // NINTENDO_ES_KERNEL_ALARM_H_INCLUDED
