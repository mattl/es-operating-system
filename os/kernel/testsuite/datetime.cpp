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

#include <string.h>
#include <es.h>
#include <es/dateTime.h>
#include <iostream>
#include <iomanip>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main(void)
{
    Object* nameSpace;
    esInit(&nameSpace);

    DateTime max = DateTime::getMaxValue().getTicks();
    DateTime min = DateTime::getMinValue().getTicks();

    bool exception = false;
    try
    {
        DateTime(-1);
    }
    catch (std::exception& e)
    {
        exception = true;
#ifdef VERBOSE
        esReport("%s\n", e.what());
#endif // VERBOSE
        TEST(strcmp(e.what(), "ticks") == 0);
    }
    TEST(exception);

    exception = false;
    try
    {
        DateTime(2000, 2, 30);
    }
    catch (std::exception& e)
    {
        exception = true;
#ifdef VERBOSE
        esReport("%s\n", e.what());
#endif // VERBOSE
        TEST(strcmp(e.what(), "date") == 0);
    }
    TEST(exception);

    const DateTime d1(2005, 5, 10, 9, 22, 8);
    TEST(d1.getYear() == 2005 &&
         d1.getMonth() == 5 &&
         d1.getDay() == 10 &&
         d1.getHour() == 9  &&
         d1.getMinute() == 22 &&
         d1.getSecond() == 8);

    DateTime d2(2004, 2, 29, 13, 42, 38);
    TEST(d2.getYear()   == 2004 &&
         d2.getMonth()  == 2 &&
         d2.getDay()    == 29 &&
         d2.getHour()   == 13 &&
         d2.getMinute() == 42 &&
         d2.getSecond() == 38);

    DateTime d3 = d2.addYears(-1);
    TEST(d3.getYear()   == 2003 &&
         d3.getMonth()  == 2 &&
         d3.getDay()    == 28 &&
         d3.getHour()   == 13 &&
         d3.getMinute() == 42 &&
         d3.getSecond() == 38);

    TEST(d3 < d2);
    TEST(d3 <= d2);
    TEST(d3 == d3);
    TEST(d3 != d2);
    TEST(d2 > d3);
    TEST(d2 >= d3);

    DateTime d4 = d3.addSeconds(10);
    TEST(d4.getYear()   == 2003 &&
         d4.getMonth()  == 2 &&
         d4.getDay()    == 28 &&
         d4.getHour()   == 13 &&
         d4.getMinute() == 42 &&
         d4.getSecond() == 48);

    DateTime tmp(1, 1, 1, 0, 10, 0);
    TimeSpan timeSpan(tmp.getTicks());
    DateTime d5 = d4 + timeSpan;
    TEST(d5.getYear()   == 2003 &&
         d5.getMonth()  == 2 &&
         d5.getDay()    == 28 &&
         d5.getHour()   == 13 &&
         d5.getMinute() == 52 &&
         d5.getSecond() == 48);

    TEST(d5 - timeSpan == d4);

    TEST(d5.getDayOfWeek() == 5);

    DateTime d6 = d5.addTicks(10000000LL);
    TEST(d6.getTicks() == d5.getTicks() + 10000000LL);

    d6 = d5.addHours(1);
    timeSpan = d6 - d5;
    TEST(timeSpan.getDay()    == 0 &&
         timeSpan.getHour()   == 1 &&
         timeSpan.getMinute() == 0 &&
         timeSpan.getSecond() == 0 &&
         timeSpan.getMillisecond() == 0);

    d6 = d5.addMinutes(1);
    timeSpan = d6 - d5;
    TEST(timeSpan.getDay()    == 0 &&
         timeSpan.getHour()   == 0 &&
         timeSpan.getMinute() == 1 &&
         timeSpan.getSecond() == 0 &&
         timeSpan.getMillisecond() == 0);

    d6 = d5.addSeconds(12);
    timeSpan = d6 - d5;
    TEST(timeSpan.getDay()    == 0 &&
         timeSpan.getHour()   == 0 &&
         timeSpan.getMinute() == 0 &&
         timeSpan.getSecond() == 12 &&
         timeSpan.getMillisecond() == 0);

    d6 = d5.addMilliseconds(10);
    timeSpan = d6 - d5;
    TEST(timeSpan.getDay()    == 0 &&
         timeSpan.getHour()   == 0 &&
         timeSpan.getMinute() == 0 &&
         timeSpan.getSecond() == 0 &&
         timeSpan.getMillisecond() == 10);

    d6 = d5.addMonths(11);
    timeSpan = d6 - d5;
    TEST(timeSpan.getDay()    == 365 - 31 &&
         timeSpan.getHour()   == 0 &&
         timeSpan.getMinute() == 0 &&
         timeSpan.getSecond() == 0 &&
         timeSpan.getMillisecond() == 0);

    d6 = d5.addDays(31);
    timeSpan = d6 - d5;
    TEST(timeSpan.getDay()    == 31 &&
         timeSpan.getHour()   == 0 &&
         timeSpan.getMinute() == 0 &&
         timeSpan.getSecond() == 0 &&
         timeSpan.getMillisecond() == 0);

    d6 = d5.addYears(10);
    TEST(d5.getYear() + 10  == d6.getYear()   &&
         d5.getMonth()  == d6.getMonth()  &&
         d5.getDay()    == d6.getDay()    &&
         d5.getHour()   == d6.getHour()   &&
         d5.getMinute() == d6.getMinute() &&
         d5.getSecond() == d6.getSecond() &&
         d5.getMillisecond() == d6.getMillisecond());

    d6 = d5.addDays(10);
    timeSpan = TimeSpan(10, 0, 0, 0);
    TEST(d6 - timeSpan == d5);
    TEST(d6 == d5 + timeSpan);

    DateTime d(DateTime::getNow());
    esReport("%d/%d/%d %02d:%02d:%02d\n",
         d.getYear(),
         d.getMonth(),
         d.getDay(),
         d.getHour(),
         d.getMinute(),
         d.getSecond());

    // TimeSpan
    TimeSpan t1(2, 3, 4);
    TimeSpan t2(1, 2, 3, 4);
    TimeSpan t3(1, 2, 3, 4, 5);

    // check operators.
    timeSpan = t2 - t1;
    TEST(timeSpan.getDay() == 1);

    timeSpan = t2 - t3;
    TEST(timeSpan.getMillisecond() == -5);
    TEST(t2 = timeSpan + t3);

    t2 = -timeSpan;
    TEST(t2.getTicks() == -timeSpan.getTicks());

    timeSpan = t3 * 2;
    TEST(timeSpan.getTicks() == 2 * t3.getTicks());

    timeSpan = t3 / 2;
    TEST(timeSpan.getTicks() == t3.getTicks() / 2);

    // conversion
    TimeSpan day(1, 0, 0, 0);
    TimeSpan hoursPerDay(24, 0, 0);

    TEST(hoursPerDay == day);

    TimeSpan week(7, 1, 0, 0);
    TimeSpan hour(1, 0, 0);
    TimeSpan minute(0, 1, 0);
    TimeSpan second(0, 0, 1);
    TimeSpan millisecond(0, 0, 0, 0, 1);

    TEST(week / day == 7);
    TEST(week % day == hour);
    TEST(minute * 60 == hour);
    TEST(second * 60 == minute);
    TEST(millisecond * 1000 == second);

    TimeSpan microsecond(0, 0, 0, 0, 0, 1);
    TEST(microsecond.getMicrosecond() == 1);
    TEST(1000 * microsecond == millisecond);

    TimeSpan maxTimeSpan(0x7FFFFFFFFFFFFFFFLL);
    TEST(maxTimeSpan.getDay() == 10675199);
    TEST(maxTimeSpan.getHour() == 2);
    TEST(maxTimeSpan.getMinute() == 48);
    TEST(maxTimeSpan.getSecond() == 5);
    TEST(maxTimeSpan.getMillisecond() == 477);
    TEST(maxTimeSpan.getMicrosecond() == 580);

    TimeSpan minTimeSpan(0x8000000000000000LL);
    TEST(minTimeSpan.getDay() == -10675199);
    TEST(minTimeSpan.getHour() == -2);
    TEST(minTimeSpan.getMinute() == -48);
    TEST(minTimeSpan.getSecond() == -5);
    TEST(minTimeSpan.getMillisecond() == -477);
    TEST(minTimeSpan.getMicrosecond() == -580);

    TimeSpan ts0 = 10;
    TimeSpan ts1 = 30;
    TEST(ts0.getMicrosecond() == 1);
    ts0 = ts1;
    TEST(ts0.getMicrosecond() == 3);

    esReport("done.\n");
}
