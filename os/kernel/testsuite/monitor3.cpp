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

static volatile int SharedResource;
static Monitor SynchroMonitor;

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void* test3(void*)
{
    int i;
    for (i = 0; i < 20000 ; ++i)
    {
        Monitor::Synchronized method(SynchroMonitor);
        ++SharedResource;
    }
    return 0;
}

int main()
{
    IInterface* root = NULL;
    esInit(&root);

    // check synchronized block.
    IThread* thread3 = new Thread(test3,            // thread function
                                  0,                // argument to thread function
                                  IThread::Normal); // priority

    IThread* thread4 = new Thread(test3,            // thread function
                                  0,                // argument to thread function
                                  IThread::Normal); // priority
    SharedResource = 0;
    void* val;
    thread3->start();
    thread4->start();

    thread3->join(&val);
    TEST(val == 0);
    thread4->join(&val);
    TEST(val == 0);

    long count;
    count = thread3->release();
    TEST(count == 0);
    count = thread4->release();
    TEST(count == 0);
    TEST(SharedResource == 20000 * 2);

    esReport("done.\n");
}
