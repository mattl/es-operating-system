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
