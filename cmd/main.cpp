/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

#include <pthread.h>
#include <setjmp.h>
#include <stdlib.h>
#include <unwind.h>
#include <es.h>
#include <es/exception.h>
#include <es/base/IProcess.h>



es::CurrentProcess* System();

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

es::Monitor* m;

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
    es::CurrentThread* current(System()->currentThread());
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

    System()->trace(true);

    m = System()->createMonitor();

    es::Thread* thread = System()->createThread((void*) start, 0);
    thread->start();

    es::CurrentThread* current(System()->currentThread());
    current->sleep(30000000);
    // System()->exit(0);

    esReport("m->notify();\n");
    m->notify();
    current->sleep(30000000);

    void* rval = thread->join();
    thread->release();

    esReport("main(): %x %x %x\n", testA, testB, &testA);

#if 1
    current = reinterpret_cast<es::CurrentThread*>(current->queryInterface(Object::iid()));
    try
    {
        current->sleep(30000000);   // Should raise an exception.
    }
    catch (Exception& error)
    {
        esReport("catch error: %d\n", error.getResult());
    }
#endif

    esReport("hello, world.\n");
}
