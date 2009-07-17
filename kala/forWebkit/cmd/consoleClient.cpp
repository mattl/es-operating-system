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
#include <es/dateTime.h>
#include <es/base/IStream.h>
#include <es/base/IProcess.h>
#include <es/usage.h>
#include <string.h>
#include <es/base/IService.h>

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

es::CurrentProcess* System();

static int CR = '\r';
static int LF = '\n';
static int BS = 0x08;

static bool done = false;

void command(Handle<es::Stream> console, char* buf, int size)
{
    int len = strlen("echo ");
    if (len < size && memcmp("echo ", buf, len) == 0)
    {
        console->write(buf + len, size - len);
        console->write("\n", 1);
    }
    else if (memcmp("date", buf, strlen("date")) == 0 )
    {
        char date[32];
        DateTime d(DateTime::getNow());
        sprintf(date, "%d/%d/%d %02d:%02d:%02d\n",
         d.getYear(),
         d.getMonth(),
         d.getDay(),
         d.getHour(),
         d.getMinute(),
         d.getSecond());
        console->write(date, strlen(date));
    }
    else if (memcmp("exit", buf, 4) == 0)
    {
        char message[] = "suspend this console, then exit the client process.\n";
        console->write(message, strlen(message));
        Handle<es::Service> service = console;
        ASSERT(service);
        service->stop();

        char warning[] = "*** This message must not be shown on the console ***\n";
        console->write(warning, strlen(warning));
        done = true; // exit this process.
    }
    else
    {
        char error[] = ": command not found\n";
        console->write(buf, size);
        console->write(error, strlen(error));
    }
}

int main(int argc, char* argv[])
{
    esReport("This is the console client process.\n");

    // System()->trace(true);

    Handle<es::CurrentThread> currentThread = System()->currentThread();
    Handle<es::Context> nameSpace = System()->getRoot();

    // check if the console is ready.
    Handle<es::Stream> console = 0;
    while (!console)
    {
        console = nameSpace->lookup("device/console");
        currentThread->sleep(10000000 / 60);
    }

    esReport("start console client loop.\n");

    // show a message on the console.
    char data[] = "This console supports echo, date and exit commands.\n";
    console->write(data, strlen(data));

    char japanese[] = "これはテストです。";
    console->write(japanese, strlen(japanese));

    Handle<es::Service> service = console;
    TEST(service);
    service->stop();
    service->start();

    char* ptr;
    char buf[1024];
    int offset = 0;
    while (!done)
    {
        int ret = console->read(buf + offset, sizeof(buf) - offset);
        if (0 < ret)
        {
            if (sizeof(buf) < offset + ret)
            {
                offset = 0; // reset
                continue;
            }
            buf[offset + ret] = 0;
            if (ptr = strchr(buf, CR))
            {
                *ptr = LF;
            }
            console->write(buf + offset, ret); // echo
            offset += ret;
            while ((ptr = strchr(buf, BS)) && 0 < offset)
            {
                *ptr = 0;
                offset = ptr - buf - 1;
            }

            if (ptr = strchr(buf, LF))
            {
                *ptr = 0;
                esReport("\"%s\"\n", buf);
                if (0 < --offset)
                {
                    command(console, buf, offset); // issue a command.
                }
                offset = 0;
            }
        }
        currentThread->sleep(10000000 / 60);
    }

    // System()->trace(false);
}
