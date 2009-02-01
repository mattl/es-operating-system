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

#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include <es/base/IProcess.h>
#include <es/usage.h>
#include "eventManager.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

es::CurrentProcess* System();

int main(int argc, char* argv[])
{
    esReport("This is the Event manager client process.\n");

    // System()->trace(true);
    Handle<es::CurrentThread> currentThread = System()->currentThread();

    Handle<es::Context> nameSpace = System()->getRoot();
    Handle<es::EventQueue> eventQueue = 0;

    while (!eventQueue)
    {
        eventQueue = nameSpace->lookup("device/event");
        currentThread->sleep(10000000 / 60);
    }

    esReport("start client loop.\n");

    unsigned int point;
    int x0;
    int y0;
    int x;
    int y;
    point = eventQueue->getMousePoint();
    x0 = point >> 16;
    y0 = point & 0xffff;
    for (;;)
    {
        if (int stroke = eventQueue->getKeystroke())
        {
            esReport("0x%x\n", stroke);
        }
        point = eventQueue->getMousePoint();
        x = point >> 16;
        y = point & 0xffff;
        if (x != x0 || y != y0)
        {
            x0 = x;
            y0 = y;
            esReport("(%3d, %3d)\n", x0, y0);
        }

        currentThread->sleep(10000000 / 60);
    }

    // System()->trace(false);
}
