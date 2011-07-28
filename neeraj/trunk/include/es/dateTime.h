/*
 * Copyright 2008 Google Inc.
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

#ifndef NINTENDO_ES_DATETIME_H_INCLUDED
#define NINTENDO_ES_DATETIME_H_INCLUDED

#include <stdexcept>
#include <es/types.h>
#include <es/timeSpan.h>

/**
 * Days of a week.
 */
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


/**
 * This class represetns date and time.
 * The epoch is 00:00:00 UTC 1 Jan 1.
 */
class DateTime
{
    /**
     * A number of 100-nanosecond ticks.
     */
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
    /**
     * Creates a date time object which represents the epoch.
     */
    DateTime() :
        ticks(0)
    {
    }
    /**
     * Creates a date time object which represents the specified ticks from the epoch.
     * @param ticks the number of 100-nanosecond ticks that have elased since the epoch.
     */
    DateTime(s64 ticks) :
        ticks(ticks)
    {
        if (ticks < MIN_TICKS || MAX_TICKS < ticks)
        {
            throw std::out_of_range("ticks");
        }
    }

    /**
     * Creates a date time object which represents the specified date.
     * @param year the year.
     * @param month the month.
     * @param day the day.
     */
    DateTime(int year, int month, int day) :
        ticks(dateToTicks(year, month, day))
    {
    }

    /**
     * Creates a date time object which represents the specified date and time.
     * @param year the year.
     * @param month the month.
     * @param day the day.
     * @param hour the hour.
     * @param minute the minute.
     * @param second the second.
     * @param millisecond the millisecond.
     * @param microsecond the microsecond.
     */
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

    /**
     * Gets the ticks of this object.
     * @retrun the number of 100-nanosecond ticks from the epoch.
     */
    s64 getTicks() const
    {
        return ticks;
    }

    /**
     * Gets the hour of this object.
     * @retrun the hour.
     */
    int getHour() const
    {
        return static_cast<int>((ticks / TICKS_PER_HOUR) % 24);
    }

    /**
     * Gets the minutes of this object.
     * @retrun the minutes.
     */
    int getMinute() const
    {
        return static_cast<int>((ticks / TICKS_PER_MINUTE) % 60);
    }

    /**
     * Gets the seconds of this object.
     * @retrun the seconds.
     */
    int getSecond() const
    {
        return static_cast<int>((ticks / TICKS_PER_SECOND) % 60);
    }

    /**
     * Gets the milliseconds of this object.
     * @retrun the milliseconds.
     */
    int getMillisecond() const
    {
        return static_cast<int>((ticks / TICKS_PER_MILLISECOND) % 1000);
    }

    /**
     * Gets the microseconds of this object.
     * @retrun the microseconds.
     */
    int getMicrosecond() const
    {
        return static_cast<int>((ticks / TICKS_PER_MICROSECOND) % 1000);
    }

    /**
     * Gets the year of this object.
     * @retrun the year.
     */
    int getYear() const
    {
        int year, month, day, yday;
        getDates(year, month, day, yday);
        return year;
    }

    /**
     * Gets the day of the year of this object.
     * @retrun the day of the year.
     * @see DayOfWeek
     */
    int getDayOfYear() const
    {
        int year, month, day, yday;
        getDates(year, month, day, yday);
        return yday;
    }

    /**
     * Gets the month of this object.
     * @retrun the month.
     */
    int getMonth() const
    {
        int year, month, day, yday;
        getDates(year, month, day, yday);
        return month;
    }

    /**
     * Gets the day of this object.
     * @retrun the number of the day.
     */
    int getDay() const
    {
        int year, month, day, yday;
        getDates(year, month, day, yday);
        return day;
    }

    /**
     * Gets the day of the week of this object.
     * @retrun the number of the day of the week.
     */
    DayOfWeek getDayOfWeek() const
    {
        return static_cast<DayOfWeek>((ticks / TICKS_PER_DAY + 1) % 7);
    }

    /**
     * Adds the specified time span to this object.
     */
    DateTime& operator+=(TimeSpan d)
    {
        ticks += d.getTicks();
        if (ticks < MIN_TICKS || MAX_TICKS < ticks)
        {
            throw std::out_of_range("ticks");
        }
        return *this;
    }

    /**
     * Subtracts the specified time span from this object.
     */
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
    /**
     * Adds the specified number of ticks to this object.
     * @param value the number of 100-nanosecond ticks.
     * @return a date time object created as the result of the addition.
     */
    DateTime addTicks(s64 value) const
    {
        return DateTime(ticks + value);
    }

    /**
     * Adds the specified number of hours to this object.
     * @param value the number of hours.
     * @return a date time object created as the result of the addition.
     */
    DateTime addHours(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_HOUR);
    }

    /**
     * Adds the specified number of minutes to this object.
     * @param value the number of minutes.
     * @return a date time object created as the result of the addition.
     */
    DateTime addMinutes(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_MINUTE);
    }

    /**
     * Adds the specified number of seconds to this object.
     * @param value the number of seconds.
     * @return a date time object created as the result of the addition.
     */
    DateTime addSeconds(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_SECOND);
    }

    /**
     * Adds the specified number of milliseconds to this object.
     * @param value the number of milliseconds.
     * @return a date time object created as the result of the addition.
     */
    DateTime addMilliseconds(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_MILLISECOND);
    }

    /**
     * Adds the specified number of microseconds to this object.
     * @param value the number of microseconds.
     * @return a date time object created as the result of the addition.
     */
    DateTime addMicroseconds(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_MICROSECOND);
    }

    /**
     * Adds the specified number of years to this object.
     * @param value the number of years.
     * @return a date time object created as the result of the addition.
     */
    DateTime addYears(int value) const
    {
        return addMonths(value * 12);
    }

    /**
     * Adds the specified number of months to this object.
     * @param value the number of months.
     * @return a date time object created as the result of the addition.
     */
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

    /**
     * Adds the specified number of days to this object.
     * @param value the number of days.
     * @return a date time object created as the result of the addition.
     */
    DateTime addDays(int value) const
    {
        return DateTime(ticks + value * TICKS_PER_DAY);
    }

    /**
     * Checks if is the year of this object is a leap year.
     * @param year the year.
     * @return the boolean whether the specified year is a leap year.
     */
    static bool isLeapYear(int year)
    {
        return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
    }

    /**
     * Gets the number of days in the specified month and year.
     * @param year the year.
     * @param month the month.
     * @return the number of days in the month.
     */
    static int daysInMonth(int year, int month)
    {
        if (month < 1 || 12 < month)
        {
            throw std::out_of_range("month");
        }
        const int* days = isLeapYear(year) ? leapYearDays : yearDays;
        return days[month] - days[month - 1];
    }

    /**
     *  Gets the maximum value of ticks.
     */
    static DateTime getMaxValue()
    {
        return DateTime(MAX_TICKS);
    }

    /**
     *  Gets the minimum value of ticks.
     */
    static DateTime getMinValue()
    {
        return DateTime(MIN_TICKS);
    }

    /**
     *  Gets a date time object which represents the current date and time.
     */
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
