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
#include <fstream>
#include <sstream>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void* child1(void* param)
{
    es::Monitor** monitor = (es::Monitor**) param;

    monitor[1]->lock();
    monitor[0]->lock();
    monitor[1]->notifyAll();
    monitor[1]->unlock();
    TEST(monitor[0]->wait());

    monitor[1]->lock();
    monitor[1]->notifyAll();
    monitor[1]->unlock();
    TEST(monitor[0]->wait());
    monitor[0]->unlock();

    return 0;   // lint
}

void* test1(void*)
{
    es::Monitor* monitor[2];
    monitor[0] = new Monitor();
    monitor[1] = new Monitor();
    es::Thread* thread = new Thread(child1,           // thread function
                                 monitor,          // argument to thread function
                                 es::Thread::Normal + 1); // priority

    TEST(thread->getState() == es::Thread::NEW);
    monitor[1]->lock();
    thread->start();
    TEST(monitor[1]->wait());

    monitor[0]->lock();
    monitor[0]->notifyAll();
    monitor[0]->unlock();
    TEST(monitor[1]->wait());
    monitor[1]->unlock();

    monitor[0]->lock();
    monitor[0]->notify();
    monitor[0]->unlock();

    void* val = thread->join();
    TEST(thread->getState() == es::Thread::TERMINATED);
    thread->release();
    TEST(val == 0);
    monitor[0]->release();
    monitor[1]->release();

    return 0;
}

int main()
{
    Object* root = NULL;
    esInit(&root);

    // check tryLock()
    es::Thread* thread1 = new Thread(test1,            // thread function
                                  0,                // argument to thread function
                                  es::Thread::Normal); // priority
    thread1->start();
    void* val = thread1->join();
    TEST(val == 0);
    long count;
    count = thread1->release();
    TEST(count == 0);

    esReport("done.\n");
}
