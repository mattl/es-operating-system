/*
 * Copyright 2008 Google Inc.
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
#include <fstream>
#include <sstream>
#include "core.h"
#include <es/dateTime.h>

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static int Shared;

void* child0(void* param)
{
    IMonitor** monitor = (IMonitor**) param;

    monitor[0]->lock();
    monitor[1]->lock();
    Shared = 1;
    monitor[1]->unlock();
    monitor[0]->unlock();

    return 0;   // lint
}

void* test0(void*)
{
    IMonitor* monitor[2];
    monitor[0] = new Monitor();
    monitor[1] = new Monitor();
    IThread* thread = new Thread(child0,           // thread function
                                 monitor,          // argument to thread function
                                 IThread::Normal + 1); // priority
    monitor[0]->lock();
    monitor[0]->lock();
    monitor[0]->lock();
    Shared = 0;

    TEST(thread->getState() == IThread::NEW);
    thread->start();

    monitor[0]->unlock();
    monitor[0]->unlock();
    monitor[0]->unlock();

    for (;;)
    {
        monitor[1]->lock();
        if (Shared == 1)
        {
            break;
        }
        monitor[1]->unlock();
    }

    void* val;
    thread->join(&val);
    TEST(thread->getState() == IThread::TERMINATED);
    thread->release();
    TEST(val == 0);
    monitor[0]->release();
    monitor[1]->release();

    return 0;
}

int main()
{
    IInterface* root = NULL;
    esInit(&root);

    IThread* thread0 = new Thread(test0,            // thread function
                                  0,                // argument to thread function
                                  IThread::Normal); // priority
    thread0->start();
    void* val;
    thread0->join(&val);
    TEST(val == 0);
    long count;
    count = thread0->release();
    TEST(count == 0);

    esReport("done.\n");
}
