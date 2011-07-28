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
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static es::Thread* ThreadHi;
static es::Thread* ThreadMid;
static es::Monitor* MonitorA;
static bool Flag = false;

static void* Hi(void* param)
{
    #pragma unused( param )

    esReport("Hi!\n");
    bool locked = MonitorA->tryLock();
    TEST(locked == false);

    MonitorA->lock();
    MonitorA->lock();
    MonitorA->unlock();
    MonitorA->unlock();
    Flag = true;

    esReport("Hi, done!\n");
    return 0;
}

static void* Mid(void* param)
{
    #pragma unused( param )
    esReport("Mid!\n");

    ThreadHi->start();
    while (!Flag)
    {
        ;
    }

    esReport("Mid, done!\n");
    return 0;
}

static void* Lo(void* param)
{
    #pragma unused( param )
    esReport("Lo!\n");

    bool locked = MonitorA->tryLock();
    TEST(locked);
    ThreadMid->start();

    // Wait for 5 seconds.
    esSleep(50000000);

    MonitorA->unlock();

    esReport("Lo, done!\n");
    return 0;
}

int main()
{
    MonitorA = new Monitor();
    TEST(MonitorA);

    Object* root = NULL;
    esInit(&root);

    ThreadHi = new Thread(Hi,                 // thread function
                          0,                  // argument to thread function
                          es::Thread::Normal+2); // priority

    ThreadMid = new Thread(Mid,               // thread function
                           0,                 // argument to thread function
                           es::Thread::Normal+1);// priority
    Lo(0);

    void* val;
    val = ThreadHi->join();
    val = ThreadMid->join();

    ThreadHi->release();
    ThreadMid->release();

    MonitorA->release();
    esReport("done.\n");
}
