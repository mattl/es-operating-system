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
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

u8 Stack[4 * 1024];

void* infiniteLoop(void*)
{
    for (;;)
    {
        ;
    }
    return 0;
}

int main()
{
    IInterface* root = NULL;
    esInit(&root);

    IThread* thread;
    thread = new Thread(infiniteLoop,     // thread function
                        0,                // argument to thread function
                        IThread::Normal - 1, // priority
                        Stack,            // stack
                        sizeof(Stack));   // stack size

    thread->start();
    TEST(thread->getState() == IThread::RUNNABLE);
    thread->cancel();
    // TEST(thread->getState() == IThread::TERMINATED);
    thread->release();

    esReport("done.\n");
}
