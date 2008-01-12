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
#include <fstream>
#include <sstream>
#include "thread.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void* child1(void* param)
{
    IMonitor** monitor = (IMonitor**) param;

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
    IMonitor* monitor[2];
    monitor[0] = new Monitor();
    monitor[1] = new Monitor();
    IThread* thread = new Thread(child1,           // thread function
                                 monitor,          // argument to thread function
                                 IThread::Normal + 1); // priority

    TEST(thread->getState() == IThread::NEW);
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

    // check tryLock()
    IThread* thread1 = new Thread(test1,            // thread function
                                  0,                // argument to thread function
                                  IThread::Normal); // priority
    thread1->start();
    void* val;
    thread1->join(&val);
    TEST(val == 0);
    long count;
    count = thread1->release();
    TEST(count == 0);

    esReport("done.\n");
}
