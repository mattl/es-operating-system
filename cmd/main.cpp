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

#include <setjmp.h>
#include <stdlib.h>
#include <unwind.h>
#include <es.h>
#include <es/exception.h>
#include <es/base/IProcess.h>

ICurrentProcess* System();

__thread int testA = 3;
__thread int testB;

class Check
{
public:
    Check()
    {
        esReport("constructed.\n");
    }
    ~Check()
    {
        esReport("destructed.\n");
    }
};

void dtor(void* ptr)
{
    esReport("dtor(%p);\n", ptr);
}

void* start(void* param)
{
    Check check;

    esReport("start(): %d %d %p\n", testA, testB, &testA);
    testA = 4;
    testB = 5;
    esReport("start(): %d %d %p\n", testA, testB, &testA);

    unsigned int key;
    esCreateThreadKey(&key, dtor);
    esSetThreadSpecific(key, (void*) 0x1234);

#if 0
    IMonitor* m = System()->createMonitor();
    m->lock();
    esReport("m->wait();\n");
    m->wait();
    m->unlock();
#endif

#if 1
    esReport("Let's unwind.\n");
    ICurrentThread* current(System()->currentThread());
    current->exit(0);

    esPanic(__FILE__, __LINE__, "Ooooops\n");
#endif
    return 0;   // lint
}

int main(int argc, char* argv[])
{
    esReport("main(%d, %p)\n", argc, argv);
    while (0 < argc--)
    {
        esReport("%s\n", *argv++);
    }

    esReport("%d %d %p\n", testA, testB, &testA);

    IThread* thread = System()->createThread(start, 0);
    thread->start();

    ICurrentThread* current(System()->currentThread());
    current->sleep(30000000);
    // System()->exit(0);

    void* rval;
    thread->join(&rval);
    thread->release();

    esReport("main(): %d %d %p\n", testA, testB, &testA);

    current->queryInterface(IID_IInterface, (void**) &current);
    try
    {
        current->sleep(30000000);   // Should raise an exception.
    }
    catch (Exception& error)
    {
        esReport("catch error: %d\n", error.getResult());
    }

    esReport("hello, world.\n");
}
