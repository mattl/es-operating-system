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

#include <es.h>
#include <fstream>
#include <sstream>
#include "core.h"
#include <es/dateTime.h>

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void* child2(void* param)
{
    IMonitor** monitor = (IMonitor**) param;

    monitor[1]->lock();
    monitor[0]->lock();
    monitor[1]->notifyAll();
    monitor[1]->unlock();
#ifdef VERBOSE
    esReport("wait\n");
#endif // VERBOSE
    monitor[0]->wait(10000000);
    monitor[0]->unlock();
    return 0;   // lint
}

void* child2timeout(void* param)
{
    IMonitor** monitor = (IMonitor**) param;
    monitor[0]->lock();
    DateTime start = DateTime::getNow();
    monitor[0]->wait(10000000);

    DateTime end = DateTime::getNow();
#ifdef VERBOSE
    DateTime diff = end - start;
    esReport("end - start =  %d.%dsec\n", diff.getSecond(), diff.getMillisecond());
#endif // VERBOSE
    DateTime upper = start.addMilliseconds(1000 + 50);
    DateTime lower = start.addMilliseconds(1000 - 50);
    TEST(lower < end && end < upper);
    monitor[0]->unlock();

    return 0;   // lint
}

void* test2(void* id)
{
    void* val;
    long count;
    IMonitor* monitor[2];
    IThread* thread;

    esReport("'%s'\n", id);

    // check timeout.
    monitor[0] = new Monitor();
    monitor[1] = new Monitor();

    monitor[1]->lock();
    thread = new Thread(child2timeout,    // thread function
                        monitor,          // argument to thread function
                        IThread::Normal); // priority
    thread->start();

    thread->join(&val);
    thread->release();
    TEST(val == 0);
    count = monitor[0]->release();
    TEST(count == 0);
    monitor[1]->unlock();
    count = monitor[1]->release();
    TEST(count == 0);
    monitor[0] = new Monitor();
    monitor[1] = new Monitor();

    // check if notifyAll() restarts a waiting thread.
    monitor[1]->lock();
    thread = new Thread(child2,           // thread function
                        monitor,          // argument to thread function
                        IThread::Normal); // priority
    thread->start();
    monitor[1]->wait();

    monitor[0]->lock();
#ifdef VERBOSE
    esReport("notifyAll\n");
#endif // VERBOSE
    TEST(thread->getState() != IThread::TERMINATED);
    monitor[0]->notifyAll();
    monitor[0]->unlock();
    monitor[1]->unlock();

    thread->join(&val);
    thread->release();
    TEST(val == 0);
    count = monitor[0]->release();
    TEST(count == 0);
    count = monitor[1]->release();
    TEST(count == 0);

    return 0;
}

char* id = "hello.";

int main()
{
    IInterface* root = NULL;
    esInit(&root);

    // check wait(s64 timeout).
    IThread* thread2 = new Thread(test2,            // thread function
                                  id,               // argument to thread function
                                  IThread::Normal); // priority
    thread2->start();
    void* val;
    thread2->join(&val);
    TEST(val == 0);
    long count;
    count = thread2->release();
    TEST(count == 0);

    esReport("done.\n");
}
