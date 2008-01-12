/*
 * Copyright (c) 2006
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
    IInterface* nameSpace = 0;
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
