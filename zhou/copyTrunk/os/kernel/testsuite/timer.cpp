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
#include <es/timer.h>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

class Tick : public TimerTask
{
public:
    void run()
    {
        DateTime t = DateTime::getNow();
        esReport("%d/%d/%d %02d:%02d:%02d\n",
                 t.getYear(),
                 t.getMonth(),
                 t.getDay(),
                 t.getHour(),
                 t.getMinute(),
                 t.getSecond());
    }
};

int main()
{
    Object* nameSpace = 0;
    esInit(&nameSpace);

    Tick task;
    task.run();
    Timer timer;
    timer.schedule(&task, DateTime::getNow() + 10000000, 10000000);

    for (int i = 0; i < 3; ++i)
    {
        esSleep(10000000);
    }


    timer.cancel(&task);

    for (int i = 0; i < 3; ++i)
    {
        esReport(".\n");
        esSleep(10000000);
    }

    esReport("done.\n");
}
