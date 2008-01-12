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

#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include <es/base/IClassStore.h>
#include <es/base/IProcess.h>
#include <es/usage.h>
#include "eventManager.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

ICurrentProcess* System();

int main(int argc, char* argv[])
{
    esReport("This is the Event manager client process.\n");

    // System()->trace(true);
    Handle<ICurrentThread> currentThread = System()->currentThread();

    Handle<IContext> nameSpace = System()->getRoot();
    Handle<IEventQueue> eventQueue = 0;

    while (!eventQueue)
    {
        eventQueue = nameSpace->lookup("device/event");
        currentThread->sleep(10000000 / 60);
    }

    esReport("start client loop.\n");

    int x0;
    int y0;
    int x;
    int y;
    eventQueue->getMousePoint(x0, y0);
    for (;;)
    {
        int stroke;
        if (eventQueue->getKeystroke(&stroke))
        {
            esReport("0x%x\n", stroke);
        }
        eventQueue->getMousePoint(x, y);
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
