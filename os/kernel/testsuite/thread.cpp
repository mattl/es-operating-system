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
    TEST(thread->getState() == IThread::RUNNABLE);
    void* val;
    thread->join(&val);
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
    void* val;
    thread->join(&val);
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
    thread->join(&val);
    TEST(val == (void*) &param && *((int*) val) == 11);
    thread->release();

    esReport("done.\n");
}
