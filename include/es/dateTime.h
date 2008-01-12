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

#ifndef NINTENDO_ES_DATETIME_H_INCLUDED
#define NINTENDO_ES_DATETIME_H_INCLUDED

#include <stdexcept>
#include <es/types.h>
#include <es/timeSpan.h>

enum DayOfWeek
{
    Sunday,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday
};

class DateTime
{
    s64 ticks;

    static const s64 TICKS_PER_MICROSECOND = 10;
    static const s64 TICKS_PER_MILLISECOND = TICKS_PER_MICROSECOND * 1000;
    static const s64 TICKS_PER_SECOND = TICKS_PER_MILLISECOND * 1000;
    static const s64 TICKS_PER_MINUTE = TICKS_PER_SECOND * 60;
    static const s64 TICKS_PER_HOUR = TICKS_PER_MINUTE * 60;
    static const s64 TICKS_PER_DAY = TICKS_PER_HOUR * 24;

    static const int DAYS_PER_YEAR = 365;
    static const int DAYS_PER_4YEARS = DAYS_PER_YEAR * 4 + 1;
    static const int DAYS_PER_100YEARS = DAYS_PER_4YEARS * 25 - 1;
    static const int DAYS_PER_400YEARS = DAYS_PER_100YEARS * 4 + 1;

    static const int yearDays[13];
    static const int leapYearDays[13];

    static const s64 MAX_TICKS = (DAYS_PER_400YEARS * 25LL - 366) * TICKS_PER_DAY - 1;
    static const s64 MIN_TICKS = 0;

    static s64 dateToTicks(int year, int month, int day)
    {
        if (year < 1 || 9999 < year)
        {
            throw std::out_of_range("year");
        }
        if (month < 1 || 12 < month)
        {
            throw std::out_of_range("month");
        }
        const int* days = isLeapYear(year)? leapYearDays : yearDays;
        if (day < 1 || days[month] - days[month - 1] < day)
        {
            throw std::out_of_range("date");
        }
        int y = year - 1;
        int n = y * 365 + y / 4 - y / 100 + y / 400 + days[month - 1] + day - 1;
        return n * TICKS_PER_DAY;
    }

    static s64 timeToTicks(int hour, int minute, int second)
    {
        if (hour < 0 || 24 <= hour)
        {
            throw std::out_of_range("hour");
        }
        if (minute < 0 || 60 <= minute)
        {
            throw std::out_of_range("minute");
        }
        if (second < 0 || 60 <= second)
        {
            throw std::out_of_range("second");
        }
        return (hour * 3600 + minute * 60 + second) * TICKS_PER_SECOND;
    }

    void getDates(int& year, int& month, int& day, int& yday) const
    {
        day = static_cast<int>(ticks / TICKS_PER_DAY);

        int y400 = day / DAYS_PER_400YEARS;
        day %= DAYS_PER_400YEARS;

        int y100 = day / DAYS_PER_100YEARS;
        if (y100 == 4)
        {
            y100 = 3;
        }
        day -= y100 * DAYS_PER_100YEARS;

        int y4 = day / DAYS_PER_4YEARS;
        day %= DAYS_PER_4YEARS;

        int y1 = day / DAYS_PER_YEAR;
        if (y1 == 4)
        {
            y1 = 3;
        }
        day -= y1 * DAYS_PER_YEAR;

        year = y400 * 400 + y100 * 100 + y4 * 4 + y1 + 1;
        yday = day + 1;
        const int* days = isLeapYear(year) ? leapYearDays : yearDays;
        month = (day >> 5) + 1;
        while (days[month] <= day)
        {
            ++month;
        }
        day -= days[month - 1] - 1;
    }

public:
    DateTime() :
        ticks(0)
    {
    }

    DateTime(s64 ticks) :
        ticks(ticks)
    {
        if (ticks < MIN_TICKS || MAX_TICKS < ticks)
        {
            throw std::out_of_range("ticks");
        }
    }

    DateTime(int year, int month, int day) :
        ticks(dateToTicks(year, month, day))
    {
    }

    DateTime(int year, int month, int day,
             int hour, int minute, int second,
             int millisecond = 0, int microsecond = 0) :
        ticks(dateToTicks(year, month, day) +
              timeToTicks(hour, minute, second) +
              millisecond * TICKS_PER_MILLISECOND +
              microsecond * TICKS_PER_MICROSECOND)
    {
        if (millisecond < 0 || 1000 <= millisecond)
        {
            throw std::out_of_range("millisecond");
        }
        if (microsecond < 0 || 1000 <= microsecond)
        {
            throw std::out_of_range("microsecond");
        }
    }

    s64 getTicks() const
    {
        return ticks;
    }

    int getHour() const
    {
        return static_cast<int>((ticks / TICKS_PER_HOUR) % 24);
    }

    int getMinute() const
    {
        return static_cast<int>((ticks / TICKS_PER_MINUTE) % 60);
    }

    int getSecond() const
    {
        return static_cast<int>((ticks / TICKS_PER_SECOND) % 60);
    }

    int getMillisecond() const
    {
        return static_cast<int>((ticks / TICKS_PER_MILLISECOND) % 1000);
    }

    int getMicrosecond() const
    {
        return static_cast<int>((ticks / TICKS_PER_MICROSECOND) % 1000);
    }

    int getYear() const
    {
        int year, month, day, yday;
        getDates(year, month, day, yday);
        return year;
    }

    int getDayOfYear() const
    {
        int year, month, day, yday;
        getDates(year, month, day, yday);
        return yday;
    }

    int getMonth() const
    {
        int year, month, day, yday;
        getDates(year, month, day, yday);
        return month;
    }

    int getDay() const
    {
        int year, month, day, yday;
        getDates(year, month, day, yday);
        return day;
    }

    DayOfWeek getDayOfWeek() const
    {
        return static_cast<DayOfWeek>((ticks / TICKS_PER_DAY + 1) % 7);
    }

    DateTime& operator+=(TimeSpan d)
    {
        ticks += d.getTicks();
        if (ticks < MIN_TICKS || MAX_TICKS < ticks)
        {
            throw std::out_of_range("ticks");
        }
        return *this;
    }

    DateTime& operator-=(TimeSpan d)
    {
        ticks -= d.getTicks();
        if (ticks < MIN_TICKS || MAX_TICKS < ticks)
        {
            throw std::out_of_range("ticks");
        }
        return *this;
    }

    friend bool operator==(const DateTime d1, const DateTime d2)
    {
        return d1.ticks == d2.ticks;
    }

    friend bool operator!=(const DateTime d1, const DateTime d2)
    {
        return d1.ticks != d2.ticks;
    }

    friend bool operator<(const DateTime d1, const DateTime d2)
    {
        return d1.ticks < d2.ticks;
    }

    friend bool operator<=(const DateTime d1, const DateTime d2)
    {
        return d1.ticks <= d2.ticks;
    }

    friend bool operator>(const DateTime d1, const DateTime d2)
    {
        return d1.ticks > d2.ticks;
    }

    friend bool operator>=(const DateTime d1, const DateTime d2)
    {
        return d1.ticks >= d2.ticks;
    }

    DateTime addTicks(s64 value) const
    {
        return DateTime(ticks + value);
    }

    DateTime addHours(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_HOUR);
    }

    DateTime addMinutes(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_MINUTE);
    }

    DateTime addSeconds(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_SECOND);
    }

    DateTime addMilliseconds(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_MILLISECOND);
    }

    DateTime addMicroseconds(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_MICROSECOND);
    }

    DateTime addYears(int value) const
    {
        return addMonths(value * 12);
    }

    DateTime addMonths(int months) const
    {
        if (months < -120000 || 120000 < months)
        {
            throw std::out_of_range("month");
        }
        int year, month, day, yday;
        getDates(year, month, day, yday);
        --month;
        month += months;
        year += month / 12;
        month %= 12;
        if (0 <= month)
        {
            ++month;
        }
        else
        {
            month += 13;
            --year;
        }
        int days = daysInMonth(year, month);
        if (days < day)
        {
            day = days;
        }
        return DateTime(year, month, day).addTicks(ticks % TICKS_PER_DAY);
    }

    DateTime addDays(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_DAY);
    }

    static bool isLeapYear(int year)
    {
        return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
    }

    static int daysInMonth(int year, int month)
    {
        if (month < 1 || 12 < month)
        {
            throw std::out_of_range("month");
        }
        const int* days = isLeapYear(year) ? leapYearDays : yearDays;
        return days[month] - days[month - 1];
    }

    static DateTime getMaxValue()
    {
        return DateTime(MAX_TICKS);
    }

    static DateTime getMinValue()
    {
        return DateTime(MIN_TICKS);
    }

    static DateTime getNow();
};

inline DateTime operator+(DateTime d1, TimeSpan d2)
{
    return d1 += d2;
}

inline TimeSpan operator-(DateTime d1, DateTime d2)
{
    return TimeSpan(d1.getTicks() - d2.getTicks());
}

inline DateTime& operator-(DateTime d1, TimeSpan d2)
{
    return d1 -= d2;
}

#endif  // #ifndef NINTENDO_ES_DATETIME_H_INCLUDED
