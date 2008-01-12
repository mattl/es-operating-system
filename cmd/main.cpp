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

#include <pthread.h>
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

IMonitor* m;

void* start(void* param)
{
    Check check;

    esReport("start(): %x %x %x\n", testA, testB, &testA);
    testA = 4;
    testB = 5;
    esReport("start(): %x %x %x\n", testA, testB, &testA);

    pthread_key_t key;
    pthread_key_create(&key, dtor);
    pthread_setspecific(key, (void*) 0x1234);

    esReport("m->lock();\n");
    m->lock();
    esReport("m->wait();\n");
    m->wait();
    esReport("m->unlock();\n");
    m->unlock();

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

    esReport("%x %x %x\n", testA, testB, &testA);

    System()->trace(false);

    m = System()->createMonitor();

    IThread* thread = System()->createThread(start, 0);
    thread->start();

    ICurrentThread* current(System()->currentThread());
    current->sleep(30000000);
    // System()->exit(0);

    esReport("m->notify();\n");
    m->notify();
    current->sleep(30000000);

    void* rval;
    thread->join(&rval);
    thread->release();

    esReport("main(): %x %x %x\n", testA, testB, &testA);

    // Check kernel page fault
    try
    {
        current->queryInterface(IID_IInterface, (void**) 0x8000);
    }
    catch (Exception& error)
    {
        esReport("catch error: %d\n", error.getResult());
    }

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
