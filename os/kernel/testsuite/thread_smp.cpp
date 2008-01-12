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
#include "apic.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void* test(void*)
{
    for (int i = 0; i < 20; ++i)
    {
        esReport("1:[%d]\n", Apic::getLocalApicID());
    }
    return 0;
}

int main()
{
    IInterface* root = NULL;
    esInit(&root);

    IThread* thread = new Thread(test,              // thread function
                                 0,                 // argument to pass
                                 IThread::Normal);  // priority
    thread->start();

    for (int i = 0; i < 20; ++i)
    {
        esReport("0:[%d]\n", Apic::getLocalApicID());
    }

    void* val;
    thread->join(&val);
    TEST(val == 0);
    thread->release();

    esReport("done.\n");
}
