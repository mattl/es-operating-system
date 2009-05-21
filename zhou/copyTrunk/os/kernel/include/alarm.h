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

#ifndef NINTENDO_ES_KERNEL_ALARM_H_INCLUDED
#define NINTENDO_ES_KERNEL_ALARM_H_INCLUDED

#include <es/list.h>
#include <es/ref.h>
#include <es/base/IAlarm.h>
#include "spinlock.h"

class Alarm : public es::Alarm
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
    es::Callback*  callback;
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
    long long getInterval();
    long long getStartTime();
    bool getEnabled();
    bool getPeriodic();
    void setCallback(es::Callback* callback);
    void setEnabled(bool enabled);
    void setPeriodic(bool periodic);
    void setInterval(long long interval);
    void setStartTime(long long time);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    // [Constructor]
    class Constructor : public es::Alarm::Constructor
    {
    public:
        es::Alarm* createInstance();
        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();
    };

    static void initializeConstructor();
};

#endif // NINTENDO_ES_KERNEL_ALARM_H_INCLUDED
