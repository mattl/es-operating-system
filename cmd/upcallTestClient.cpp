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

#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include <es/base/IClassStore.h>
#include <es/base/IProcess.h>
#include <es/usage.h>

using namespace es;

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

ICurrentProcess* System();

int main(int argc, char* argv[])
{
    esReport("This is the test client process.\n");

    // System()->trace(true);

    Handle<ICurrentThread> currentThread = System()->currentThread();

    Handle<IContext> nameSpace = System()->getRoot();

    Handle<IStream> server;
    while (!(server = nameSpace->lookup("device/testServer")))
    {
        // wait server for device registration
        currentThread->sleep(10000000);
    }

    char message[64];
    sprintf(message, "test message.");
    server->write(message, strlen(message) + 1);

    nameSpace->unbind("device/testServer");

    System()->trace(false);
}
