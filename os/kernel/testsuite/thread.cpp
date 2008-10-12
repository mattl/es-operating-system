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
#include <sstream>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void* test1(void*)
{
    esReport("hello, thread!\n");
    // Wait for 1 second.
    esSleep(10000000);
    return 0;
}

void* test0(void*)
{
    IThread* thread = new Thread(test1,     // thread function
                                 0,         // argument to thread function
                                 20);       // priority
    TEST(thread->getState() == IThread::NEW);

    thread->start();
    //TEST(thread->getState() == IThread::RUNNABLE);
    void* val = thread->join();
    TEST(val == 0);
    TEST(thread->getState() == IThread::TERMINATED);
    thread->release();
    return 0;
}

void* incrementValue(void* param)
{
    (*(int*) param)++;
    return param;
}

int main()
{
    IInterface* root = NULL;
    esInit(&root);

    // create, start and join.
    IThread* thread = new Thread(test0,             // thread function
                                 0,                 // argument to thread function
                                 IThread::Highest); // priority
    TEST(thread->getState() == IThread::NEW);
    TEST(thread->getPriority() == IThread::Highest);
    thread->setPriority(IThread::Normal);
    TEST(thread->getPriority() == IThread::Normal);

    thread->start();
    thread->setPriority(IThread::Normal + 1);
    TEST(thread->getPriority() == IThread::Normal + 1);
    void* val = thread->join();
    TEST(val == 0);
    TEST(thread->getState() == IThread::TERMINATED);
    thread->release();

    // pass an argument and get a result.
    int param = 10;
    thread = new Thread(incrementValue,   // thread function
                        &param,           // argument to thread function
                        IThread::Lowest); // priority
    thread->start();
    TEST(thread->getState() == IThread::RUNNABLE);
    val = thread->join();
    TEST(val == (void*) &param && *((int*) val) == 11);
    thread->release();

    esReport("done.\n");
}
