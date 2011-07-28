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

#include <es.h>
#include <sstream>
#include <es/base/ICallback.h>
#include <es/base/IAlarm.h>
#include <es/handle.h>
#include "core.h"
#include "alarm.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

class AlarmCallback : public es::Callback
{
    int ref;
public:
    AlarmCallback();

    // ICallback
    int invoke(int result);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

namespace
{
    Thread* ThreadHi;
    Thread* ThreadMid;
    es::Monitor* MonitorA;
    es::Monitor* MonitorB;
    bool Flag = false;
    int CheckPoint = 0;
};

static void* Hi(void* param)
{
    #pragma unused( param )

    // TEST(CheckPoint++ == 2);
    esReport("Hi!\n");
    ThreadMid->cancel();
    esReport("Cancel Mid.\n");
    MonitorB->lock();
    MonitorB->unlock();
    esReport("Hi, done!\n");
    // TEST(CheckPoint++ == 3);
    return 0;
}

static void* Mid(void* param)
{
    #pragma unused( param )
    esReport("Mid!\n");
    // TEST(CheckPoint++ == 0);

    MonitorB->lock();
    MonitorA->lock();
    ThreadMid->testCancel();
    // NOT REACHED HERE
    TEST(0);
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
    // TEST(CheckPoint++ == 1);
    while (!Flag)
    {
        ;
    }
    // TEST(CheckPoint++ == 4);
    MonitorA->unlock();

    esReport("Lo, done!\n");
    // TEST(CheckPoint++ == 5);
    return 0;
}

int main()
{
    Object* ns = NULL;
    esInit(&ns);

    Handle<es::Alarm> alarm = es::Alarm::createInstance();

    AlarmCallback* alarmCallback = new AlarmCallback;
    alarm->setInterval(40000000LL);
    alarm->setEnabled(true);
    alarm->setCallback(static_cast<es::Callback*>(alarmCallback));

    MonitorA = new Monitor();
    MonitorB = new Monitor();
    TEST(MonitorA && MonitorB);

    ThreadHi = new Thread(Hi,                 // thread function
                          0,                  // argument to thread function
                          es::Thread::Normal+2); // priority

    ThreadMid = new Thread(Mid,               // thread function
                           0,                 // argument to thread function
                           es::Thread::Normal+1);// priority
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

Object* AlarmCallback::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Callback::iid()) == 0)
    {
        objectPtr = static_cast<es::Callback*>(this);
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

unsigned int AlarmCallback::
addRef()
{
    return ++ref;
}

unsigned int AlarmCallback::
release()
{
    if (--ref == 0)
    {
        delete this;
        return 0;
    }
    return ref;
}
