/*
 * Copyright 2008 Google Inc.
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
