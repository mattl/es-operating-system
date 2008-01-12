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

#ifndef NINTENDO_ES_TIMESPAN_H_INCLUDED
#define NINTENDO_ES_TIMESPAN_H_INCLUDED

#include <stdexcept>
#include <es/types.h>

class TimeSpan
{
    s64 ticks;

    static const s64 TICKS_PER_MICROSECOND = 10;
    static const s64 TICKS_PER_MILLISECOND = TICKS_PER_MICROSECOND * 1000;
    static const s64 TICKS_PER_SECOND = TICKS_PER_MILLISECOND * 1000;
    static const s64 TICKS_PER_MINUTE = TICKS_PER_SECOND * 60;
    static const s64 TICKS_PER_HOUR = TICKS_PER_MINUTE * 60;
    static const s64 TICKS_PER_DAY = TICKS_PER_HOUR * 24;

    static const s64 MAX_VALUE = 9223372036854775807LL;
    static const s64 MIN_VALUE = (-MAX_VALUE - 1LL);

    static const s64 MAX_SECONDS = MAX_VALUE / TICKS_PER_SECOND;
    static const s64 MIN_SECONDS = MIN_VALUE / TICKS_PER_SECOND;
    static const s64 MAX_MILLISECONDS = MAX_VALUE / TICKS_PER_MILLISECOND;
    static const s64 MIN_MILLISECONDS = MIN_VALUE / TICKS_PER_MILLISECOND;
    static const s64 MAX_MICROSECONDS = MAX_VALUE / TICKS_PER_MICROSECOND;
    static const s64 MIN_MICROSECONDS = MIN_VALUE / TICKS_PER_MICROSECOND;

public:
    TimeSpan() : ticks(0)
    {
    }

    TimeSpan(s64 ticks) : ticks(ticks)
    {
    }

    TimeSpan(int hour, int minute, int second)
    {
        s64 total = hour * 3600LL + minute * 60LL + second;
        if (total < MIN_SECONDS || MAX_SECONDS < total)
        {
            throw std::out_of_range("ticks");
        }
        ticks = total * TICKS_PER_SECOND;
    }

    TimeSpan(int day, int hour, int minute, int second,
             int millisecond = 0)
    {
        s64 total = (day * 24 * 3600LL  + hour * 3600LL + minute * 60LL + second) * 1000 + millisecond;
        if (total < MIN_MILLISECONDS || MAX_MILLISECONDS < total)
        {
            throw std::out_of_range("ticks");
        }
        ticks = total * TICKS_PER_MILLISECOND;
    }

    TimeSpan(int day, int hour, int minute, int second,
             int millisecond, int microsecond)
    {
        s64 total = ((day * 24 * 3600LL  + hour * 3600LL + minute * 60LL + second) * 1000 + millisecond) * 1000 + microsecond;
        if (total < MIN_MICROSECONDS || MAX_MICROSECONDS < total)
        {
            throw std::out_of_range("ticks");
        }
        ticks = total * TICKS_PER_MICROSECOND;
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

    int getDay() const
    {
        return static_cast<int>((ticks / TICKS_PER_DAY));
    }

    TimeSpan addTicks(s64 value) const
    {
        return TimeSpan(ticks + value);
    }

    TimeSpan addHours(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_HOUR);
    }

    TimeSpan addMinutes(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_MINUTE);
    }

    TimeSpan addSeconds(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_SECOND);
    }

    TimeSpan addMilliseconds(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_MILLISECOND);
    }

    TimeSpan addMicroseconds(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_MICROSECOND);
    }

    TimeSpan addDays(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_DAY);
    }

    static TimeSpan getMaxValue()
    {
        return TimeSpan(MAX_VALUE);
    }

    static TimeSpan getMinValue()
    {
        return TimeSpan(MIN_VALUE);
    }

    // conversion operator
    operator s64() const
    {
        return ticks;
    }
};

#endif  // #ifndef NINTENDO_ES_TIMESPAN_H_INCLUDED
