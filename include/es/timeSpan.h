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

/**
 * This class represents a time interval.
 */
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
    /**
     * Creates a time span object which represents a zero time interval.
     */
    TimeSpan() : ticks(0)
    {
    }

    /**
     * Creates a time span object which represents the specified time interval.
     * @param ticks the number of 100-nanosecond ticks.
     */
    TimeSpan(s64 ticks) : ticks(ticks)
    {
    }

    /**
     * Creates a time span object which represents the specified time interval.
     * @param hour the number of hours.
     * @param minute the number of minutes.
     * @param second the number of seconds.
     */
    TimeSpan(int hour, int minute, int second)
    {
        s64 total = hour * 3600LL + minute * 60LL + second;
        if (total < MIN_SECONDS || MAX_SECONDS < total)
        {
            throw std::out_of_range("ticks");
        }
        ticks = total * TICKS_PER_SECOND;
    }

    /**
     * Creates a time span object which represents the specified time interval.
     * @param day the number of days.
     * @param hour the number of hours.
     * @param minute the number of minutes.
     * @param second the number of seconds.
     * @param millisecond the number of milliseconds.
     */
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

    /**
     * Creates a time span object which represents the specified time.
     * @param day the number of days.
     * @param hour the number of hours.
     * @param minute the number of minutes.
     * @param second the number of seconds.
     * @param millisecond the number of milliseconds.
     * @param microsecond the number of microseconds.
     */
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

    /**
     * Gets the number of ticks in this time interval.
     * @return the number of 100-nanosecond ticks.
     */
    s64 getTicks() const
    {
        return ticks;
    }

    /**
     * Gets the number of hours in this time interval.
     * @return the number of hours.
     */
    int getHour() const
    {
        return static_cast<int>((ticks / TICKS_PER_HOUR) % 24);
    }

    /**
     * Gets the number of minutes in this time interval.
     * @return the number of minutes.
     */
    int getMinute() const
    {
        return static_cast<int>((ticks / TICKS_PER_MINUTE) % 60);
    }

    /**
     * Gets the number of seconds in this time interval.
     * @return the number of seconds.
     */
    int getSecond() const
    {
        return static_cast<int>((ticks / TICKS_PER_SECOND) % 60);
    }

    /**
     * Gets the number of milliseconds in this time interval.
     * @return the number of milliseconds.
     */
    int getMillisecond() const
    {
        return static_cast<int>((ticks / TICKS_PER_MILLISECOND) % 1000);
    }

    /**
     * Gets the number of microseconds in this time interval.
     * @return the number of microseconds.
     */
    int getMicrosecond() const
    {
        return static_cast<int>((ticks / TICKS_PER_MICROSECOND) % 1000);
    }

    /**
     * Gets the number of days in this time interval.
     * @return the number of days.
     */
    int getDay() const
    {
        return static_cast<int>((ticks / TICKS_PER_DAY));
    }

    /**
     * Adds the specified ticks to this time interval.
     * @param value the number of ticks.
     * @return a time span object created as the result of the addition.
     */
    TimeSpan addTicks(s64 value) const
    {
        return TimeSpan(ticks + value);
    }

    /**
     * Adds the specified hours to this time interval.
     * @param value the number of hours.
     * @return a time span object created as the result of the addition.
     */
    TimeSpan addHours(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_HOUR);
    }

    /**
     * Adds the specified minutes to this time interval.
     * @param value the number of minutes.
     * @return a time span object created as the result of the addition.
     */
    TimeSpan addMinutes(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_MINUTE);
    }

    /**
     * Adds the specified seconds to this time interval.
     * @param value the number of seconds.
     * @return a time span object created as the result of the addition.
     */
    TimeSpan addSeconds(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_SECOND);
    }

    /**
     * Adds the specified milliseconds to this time interval.
     * @param value the number of milliseconds.
     * @return a time span object created as the result of the addition.
     */
    TimeSpan addMilliseconds(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_MILLISECOND);
    }

    /**
     * Adds the specified microseconds to this time interval.
     * @param value the number of microseconds.
     * @return a time span object created as the result of the addition.
     */
    TimeSpan addMicroseconds(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_MICROSECOND);
    }

    /**
     * Adds the specified days to this time interval.
     * @param value the number of days.
     * @return a time span object created as the result of the addition.
     */
    TimeSpan addDays(s64 value) const
    {
        return TimeSpan(ticks + value * TICKS_PER_DAY);
    }

    /**
     * Get the maximum time interval.
     */
    static TimeSpan getMaxValue()
    {
        return TimeSpan(MAX_VALUE);
    }

    /**
     * Get the minimum time interval (a negative value).
     */
    static TimeSpan getMinValue()
    {
        return TimeSpan(MIN_VALUE);
    }

    /**
     * A conversion operator.
     */
    operator s64() const
    {
        return ticks;
    }
};

#endif  // #ifndef NINTENDO_ES_TIMESPAN_H_INCLUDED
