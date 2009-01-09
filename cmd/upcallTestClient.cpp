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

#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
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
