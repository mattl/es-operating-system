/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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
    Object* root = NULL;
    esInit(&root);

    es::Thread* thread = new Thread(test,              // thread function
                                 0,                 // argument to pass
                                 es::Thread::Normal);  // priority
    thread->start();

    for (int i = 0; i < 20; ++i)
    {
        esReport("0:[%d]\n", Apic::getLocalApicID());
    }

    void* val = thread->join();
    TEST(val == 0);
    thread->release();

    esReport("done.\n");
}
