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
#include "thread.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static IThread* ThreadHi;
static IThread* ThreadMid;
static IMonitor* MonitorA;
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

    IInterface* root = NULL;
    esInit(&root);

    ThreadHi = new Thread(Hi,                 // thread function
                          0,                  // argument to thread function
                          IThread::Normal+2); // priority

    ThreadMid = new Thread(Mid,               // thread function
                           0,                 // argument to thread function
                           IThread::Normal+1);// priority
    Lo(0);

    void* val;
    ThreadHi->join(&val);
    ThreadMid->join(&val);

    ThreadHi->release();
    ThreadMid->release();

    MonitorA->release();
    esReport("done.\n");
}
