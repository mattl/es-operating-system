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

#include <es.h>
#include <sstream>
#include <es/base/ICallback.h>
#include <es/base/IAlarm.h>
#include <es/clsid.h>
#include <es/handle.h>
#include "core.h"
#include "alarm.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

class AlarmCallback : public ICallback
{
    int ref;
public:
    AlarmCallback();

    // ICallback
    int invoke(int result);

    // IInterface
    void* queryInterface(const Guid& riid);
    unsigned int addRef(void);
    unsigned int release(void);
};

namespace
{
    IThread* ThreadHi;
    IThread* ThreadMid;
    IMonitor* MonitorA;
    IMonitor* MonitorB;
    bool Flag = false;
};

static void* Hi(void* param)
{
    #pragma unused( param )

    esReport("Hi!\n");
    MonitorB->lock();
    MonitorB->unlock();
    esReport("Hi, done!\n");
    return 0;
}

static void* Mid(void* param)
{
    #pragma unused( param )
    esReport("Mid!\n");

    MonitorA->lock();
    MonitorB->lock();
    MonitorA->unlock();
    MonitorB->unlock();

    esReport("Mid, done!\n");
    return 0;
}

static void* Lo(void* param)
{
    #pragma unused( param )
    esReport("Lo!\n");

    MonitorA->lock();
    ThreadMid->start();
    ThreadMid->release();
    while (!Flag)
    {
        ;
    }
    MonitorA->unlock();

    esReport("Lo, done!\n");
    return 0;
}

int main()
{
    IInterface* ns = NULL;
    esInit(&ns);

    Handle<IAlarm> alarm;
    alarm = reinterpret_cast<IAlarm*>(
        esCreateInstance(CLSID_Alarm, IAlarm::iid()));

    AlarmCallback* alarmCallback = new AlarmCallback;
    alarm->setInterval(50000000LL);
    alarm->setEnabled(true);
    alarm->setCallback(static_cast<ICallback*>(alarmCallback));

    MonitorA = new Monitor();
    MonitorB = new Monitor();
    TEST(MonitorA && MonitorB);

    ThreadHi = new Thread(Hi,                 // thread function
                          0,                  // argument to thread function
                          IThread::Normal+2); // priority

    ThreadMid = new Thread(Mid,               // thread function
                           0,                 // argument to thread function
                           IThread::Normal+1);// priority
    TEST(ThreadHi && ThreadMid);

    Lo(0);

    alarm->setEnabled(false);
    delete alarmCallback;

    MonitorA->release();
    MonitorB->release();

    esReport("done.\n");
    return 0;
}

AlarmCallback::
AlarmCallback() : ref(0)
{

};

//
// ICallback
//
int AlarmCallback::
invoke(int result)
{
    Flag = true;
    TEST(ThreadHi);
    ThreadHi->start();
    ThreadHi->release();
}

//
// IInterface
//

void* AlarmCallback::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == ICallback::iid())
    {
        objectPtr = static_cast<ICallback*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<ICallback*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
}

unsigned int AlarmCallback::
addRef(void)
{
    return ++ref;
}

unsigned int AlarmCallback::
release(void)
{
    if (--ref == 0)
    {
        delete this;
        return 0;
    }
    return ref;
}
