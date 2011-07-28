/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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
#include <time.h>
#include <es/dateTime.h>
#include "esjs.h"
#include "parser.h"

namespace
{
    s64 localTZA = 9 * 60 * 60 * 1000LL; // [check] how to set local time zone adjustment?
    const DateTime Origin(1970, 1, 1, 0, 0, 0, 0, 0); // January 1, 1970 UTC.
    const int MaxDateStringSize = 32;

    enum LocaleType
    {
        TypeUtc,
        TypeLocalTime
    };

    enum TimeType
    {
        TypeFullYear,
        TypeYear,
        TypeMonth,
        TypeWeekDay,
        TypeDate,
        TypeHour,
        TypeMinute,
        TypeSecond,
        TypeMillisecond
    };

    struct CalenderTime
    {
        struct tm   t;  // year, month, ..., minute, second.
        int         ms; // millisecond.
    };

    double timeClip(double x)
    {
        if (!isfinite(x) || 8.64e15 < fabs(x))
        {
            return NAN;
        }
        if (0 <= x)
        {
            return floor(fabs(x));
        }
        else
        {
            return -floor(fabs(x));
        }
    }

    double UTC(double localtime)  // LocalTime -> UTC [millisecond]
    {
        return (localtime - static_cast<double>(localTZA)); // [check] daylight saving time
    }

    double LocalTime(double utc) // UTC -> LocalTime [millisecond]
    {
        return (utc + static_cast<double>(localTZA)); // [check] daylight saving time.
    }

    s64 getTick(CalenderTime* time, const LocaleType calenderType)
    {
        s64 tick;
        switch (calenderType)
        {
        case TypeLocalTime:
            tick = mktime(&time->t) * 1000LL;
            break;
        case TypeUtc:
            tick = mktime(&time->t) * 1000LL;
            tick += localTZA;
            break;
        default:
            throw getErrorInstance("TypeError");
            break;
        }
        return tick + time->ms;
    }

    double makeTick(double year, double month, double day, double hours, double minutes, double sec, double ms)
    {
        if (!isfinite(year) || !isfinite(month) || !isfinite(day) ||
            !isfinite(hours) || !isfinite(minutes) || !isfinite(sec) || !isfinite(ms))
        {
            return NAN;
        }
        return (DateTime(static_cast<int>(year), 1 + static_cast<int>(month), static_cast<int>(day),
                         static_cast<int>(hours), static_cast<int>(minutes), static_cast<int>(sec),
                         static_cast<int>(ms)).getTicks() -
                Origin.getTicks()) / 10000;
    }

    double makeDate()
    {
        Value* value;

        double year = getScopeChain()->get("year")->toNumber();
        double month = getScopeChain()->get("month")->toNumber();

        value = getScopeChain()->get("date");
        double date = value->isUndefined() ? 1 : value->toNumber();
        value = getScopeChain()->get("hours");
        double hours = value->isUndefined() ? 0 : value->toNumber();
        value = getScopeChain()->get("minutes");
        double minutes = value->isUndefined() ? 0 : value->toNumber();
        value = getScopeChain()->get("seconds");
        double seconds = value->isUndefined() ? 0 : value->toNumber();
        value = getScopeChain()->get("ms");
        double ms = value->isUndefined() ? 0 : value->toNumber();

        if (!isnan(year) && -0.0 <= year && year < 100.0)
        {
            year = 1900.0 + floor(year);
        }
        return makeTick(year, month, date, hours, minutes, seconds, ms);
    }

    double parse(std::string s)
    {
        struct tm t;

        if (!strptime(s.c_str(), "%c", &t))
        {
            return NAN;
        }

        return timeClip(UTC(makeTick(1900 + t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, 0)));
    }

    // tick [ms] -> CalenderTime{}
    void getCalenderTime(s64 tick, CalenderTime* time, const LocaleType type)
    {
        DateTime d(Origin.getTicks() + tick * 10000LL);

        switch (type)
        {
        case TypeLocalTime:
            d = d.addMilliseconds(localTZA);
            break;
        case TypeUtc:
            break;
        default:
            throw getErrorInstance("TypeError");
            break;
        }

        struct tm& t = time->t;
        t.tm_year = d.getYear() - 1900;
        t.tm_mon = d.getMonth()-1;
        t.tm_mday = d.getDay();
        t.tm_hour = d.getHour();
        t.tm_min = d.getMinute();
        t.tm_sec = d.getSecond();
        t.tm_wday = d.getDayOfWeek();
        t.tm_yday = d.getDayOfYear();
        t.tm_isdst = 0; // [check]
        time->ms = d.getMillisecond();
    }

    void getString(const struct tm* time, char* buf, int size, const char* format)
    {
        strftime(buf, size, format, time);
    }
};

class DateValue : public ObjectValue
{
    static ObjectValue* prototype;  // Date.prototype

    static const s64 MsPerMinute = 60 * 1000LL;

public:
    DateValue(double millisecond)
    {
        Register<ObjectValue> tmp(this);    // Not to be swept
        setPrototype(prototype);
        Register<NumberValue> time = new NumberValue(millisecond);
        setValueProperty(time);
    }

    const char* getClass() const
    {
        return "Date";
    }

    Value* toPrimitive(int hint)
    {
        if (hint == Value::UndefinedType)
        {
            hint = Value::StringType;   // 8.6.2.6
        }
        return ObjectValue::toPrimitive(hint);
    }

    int getThisCalenderTime(CalenderTime* time, const LocaleType type = TypeUtc)
    {
        Value* value = getValueProperty();
        double tick = value->toNumber();
        if (isnan(tick))
        {
            return -1;
        }

        getCalenderTime(static_cast<s64>(tick), time, type);

        return 0;
    }

    Value* toString(const char* format, const LocaleType type = TypeUtc)
    {
        CalenderTime time;
        char buf[MaxDateStringSize];
        if (getThisCalenderTime(&time, type) == 0)
        {
            getString(&time.t, buf, sizeof(buf), format);
        }
        else
        {
            sprintf(buf, "NaN");
        }

        return new StringValue(buf);
    }

    Value* getValue(const TimeType type, const LocaleType locale = TypeUtc)
    {
        CalenderTime time;
        if (getThisCalenderTime(&time, locale) < 0)
        {
            return new NumberValue(NAN);
        }

        double value;
        switch (type)
        {
        case TypeFullYear:
            value = time.t.tm_year + 1900;
            break;

        case TypeYear:
            value = time.t.tm_year;
            break;

        case TypeMonth:
            value = time.t.tm_mon;
            break;

        case TypeWeekDay:
            value = time.t.tm_wday;
            break;

        case TypeDate:
            value = time.t.tm_mday;
            break;

        case TypeHour:
            value = time.t.tm_hour;
            break;

        case TypeMinute:
            value = time.t.tm_min;
            break;

        case TypeSecond:
            value = time.t.tm_sec;
            break;

        case TypeMillisecond:
            value = time.ms;
            break;

        default:
            throw getErrorInstance("TypeError");
            break;
        }
        return new NumberValue(value);
    }

    Value* getTimezoneOffset()
    {
        Value* value = getValueProperty();
        double tick = value->toNumber();

        return new NumberValue((tick - LocalTime(tick)) / MsPerMinute);
    }

    Value* setDate(const TimeType type, const LocaleType locale = TypeUtc)
    {
        CalenderTime time;
        getThisCalenderTime(&time, locale);

        Value* year = getScopeChain()->get("year");
        Value* month = getScopeChain()->get("month");
        Value* date = getScopeChain()->get("date");

        if (!year->isUndefined())
        {
            switch (type)
            {
            case TypeFullYear:
                time.t.tm_year = static_cast<int>(year->toNumber()) - 1900;
                break;
            case TypeYear:
                time.t.tm_year = static_cast<int>(year->toNumber());
                break;
            }
        }
        if (!month->isUndefined())
        {
            time.t.tm_mon = static_cast<int>(month->toNumber());
        }
        if (!date->isUndefined())
        {
            time.t.tm_mday = static_cast<int>(date->toNumber());
        }

        return setValue(&time, locale);
    }

    Value* setTime(const TimeType type, const LocaleType locale = TypeUtc)
    {
        CalenderTime time;
        getThisCalenderTime(&time, locale);

        Value* hour = getScopeChain()->get("hour");
        Value* min = getScopeChain()->get("min");
        Value* sec = getScopeChain()->get("sec");
        Value* ms= getScopeChain()->get("ms");

        if (!hour->isUndefined())
        {
            time.t.tm_hour = static_cast<int>(hour->toNumber());
        }
        if (!min->isUndefined())
        {
            time.t.tm_min = static_cast<int>(min->toNumber());
        }
        if (!sec->isUndefined())
        {
            time.t.tm_sec = static_cast<int>(sec->toNumber());
        }
        if (!ms->isUndefined())
        {
            time.ms = static_cast<int>(ms->toNumber());
        }

        return setValue(&time, locale);
    }

    Value* setValue(CalenderTime* time, const LocaleType locale)
    {
        double tick = getTick(time, locale);
        Value* value = new NumberValue(timeClip(tick));
        setValueProperty(value);

        return value;
    }

    Value* setValue()
    {
        Value* time = getScopeChain()->get("time");
        ObjectValue* date = static_cast<ObjectValue*>(static_cast<Value*>(time));
        if (!dynamic_cast<DateValue*>(date))
        {
            throw getErrorInstance("TypeError");
        }
        Value* tick = date->getValueProperty();
        Value* value = new NumberValue(timeClip(tick->toNumber()));
        setValueProperty(value);
        return value;
    }

    friend class DateConstructor;
};

ObjectValue* DateValue::prototype;  // Date.prototype

//
// Date Methods
//

class DateMethod : public Code
{
    enum Method
    {
        ToString,
        ToDateString,
        ToTimeString,
        ToLocaleString,
        ToLocaleDateString,
        ToLocaleTimeString,
        ValueOf,
        GetTime,
        GetFullYear,
        GetUTCFullYear,
        GetMonth,
        GetUTCMonth,
        GetDate,
        GetUTCDate,
        GetDay,
        GetUTCDay,
        GetHours,
        GetUTCHours,
        GetMinutes,
        GetUTCMinutes,
        GetSeconds,
        GetUTCSeconds,
        GetMilliseconds,
        GetUTCMilliseconds,
        GetTimezoneOffset,
        SetTime,
        SetFullYear,
        SetUTCFullYear,
        SetMonth,
        SetUTCMonth,
        SetDate,
        SetUTCDate,
        SetHours,
        SetUTCHours,
        SetMinutes,
        SetUTCMinutes,
        SetSeconds,
        SetUTCSeconds,
        SetMilliseconds,
        SetUTCMilliseconds,
        ToUTCString,
        ToGMTString,
        GetYear,
        SetYear,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

public:
    DateMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);

        switch (method)
        {
        case SetTime:
            arguments->add(new Identifier("time"));
            break;
        case SetFullYear:
        case SetUTCFullYear:
            arguments->add(new Identifier("year"));
            // FALL THROUGH
        case SetMonth:
        case SetUTCMonth:
            arguments->add(new Identifier("month"));
            // FALL THROUGH
        case SetDate:
        case SetUTCDate:
            arguments->add(new Identifier("date"));
            break;

        case SetHours:
        case SetUTCHours:
            arguments->add(new Identifier("hour"));
            // FALL THROUGH
        case SetMinutes:
        case SetUTCMinutes:
            arguments->add(new Identifier("min"));
            // FALL THROUGH
        case SetSeconds:
        case SetUTCSeconds:
            arguments->add(new Identifier("sec"));
            // FALL THROUGH
        case SetMilliseconds:
        case SetUTCMilliseconds:
            arguments->add(new Identifier("ms"));
            break;
        case SetYear:
            arguments->add(new Identifier("year"));
            break;

        default:
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }

    ~DateMethod()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        DateValue* self = dynamic_cast<DateValue*>(getThis());
        if (!self)
        {
            throw getErrorInstance("TypeError");
        }

        Register<Value> value;
        switch (method)
        {
        case ToUTCString:
        case ToGMTString:
            value = self->toString("%c UTC");
            break;
        case ToString:
        case ToLocaleString:
            value = self->toString("%c", TypeLocalTime);
            break;
        case ToDateString:
        case ToLocaleDateString:
            value = self->toString("%x", TypeLocalTime);
            break;
        case ToTimeString:
        case ToLocaleTimeString:
            value = self->toString("%X", TypeLocalTime);
            break;

        case GetTime:
        case ValueOf:
            value = self->getValueProperty();
            break;

        case GetFullYear:
            value = self->getValue(TypeFullYear, TypeLocalTime);
            break;
        case GetUTCFullYear:
            value = self->getValue(TypeFullYear);
            break;
        case GetMonth:
            value = self->getValue(TypeMonth, TypeLocalTime);
            break;
        case GetUTCMonth:
            value = self->getValue(TypeMonth);
            break;
        case GetDate:
            value = self->getValue(TypeDate, TypeLocalTime);
            break;
        case GetUTCDate:
            value = self->getValue(TypeDate);
            break;
        case GetDay:
            value = self->getValue(TypeWeekDay, TypeLocalTime);
            break;
        case GetUTCDay:
            value = self->getValue(TypeWeekDay);
            break;
        case GetHours:
            value = self->getValue(TypeHour, TypeLocalTime);
            break;
        case GetUTCHours:
            value = self->getValue(TypeHour);
            break;
        case GetMinutes:
            value = self->getValue(TypeMinute, TypeLocalTime);
            break;
        case GetUTCMinutes:
            value = self->getValue(TypeMinute);
            break;
        case GetSeconds:
            value = self->getValue(TypeSecond, TypeLocalTime);
            break;
        case GetUTCSeconds:
            value = self->getValue(TypeSecond);
            break;
        case GetMilliseconds:
            value = self->getValue(TypeMillisecond, TypeLocalTime);
            break;
        case GetUTCMilliseconds:
            value = self->getValue(TypeMillisecond);
            break;

        case GetTimezoneOffset:
            value = self->getTimezoneOffset();
            break;

        case SetTime:
            value = self->setValue();
            break;

        case SetFullYear:
            value = self->setDate(TypeFullYear, TypeLocalTime);
            break;
        case SetUTCFullYear:
            value = self->setDate(TypeFullYear);
            break;
        case SetMonth:
            value = self->setDate(TypeMonth, TypeLocalTime);
            break;
        case SetUTCMonth:
            value = self->setDate(TypeMonth);
            break;
        case SetDate:
            value = self->setDate(TypeDate, TypeLocalTime);
            break;
        case SetUTCDate:
            value = self->setDate(TypeDate);
            break;

        case SetHours:
            value = self->setTime(TypeHour, TypeLocalTime);
            break;
        case SetUTCHours:
            value = self->setTime(TypeHour);
            break;
        case SetMinutes:
            value = self->setTime(TypeMinute, TypeLocalTime);
            break;
        case SetUTCMinutes:
            value = self->setTime(TypeMinute);
            break;
        case SetSeconds:
            value = self->setTime(TypeSecond, TypeLocalTime);
            break;
        case SetUTCSeconds:
            value = self->setTime(TypeSecond);
            break;
        case SetMilliseconds:
            value = self->setTime(TypeMillisecond, TypeLocalTime);
            break;
        case SetUTCMilliseconds:
            value = self->setTime(TypeMillisecond);
            break;

        //
        // Additional Properties for Compatibility [B.2]
        //
        case GetYear:
            value = self->getValue(TypeYear);
            break;
        case SetYear:
            value = self->setDate(TypeYear);
            break;

        default:
            value = UndefinedValue::getInstance();
            break;
        }
        return CompletionType(CompletionType::Return, value, "");
    }

    const char* name() const
    {
        return names[method];
    }

    static int methodCount()
    {
        return MethodCount;
    }
};

const char* DateMethod::names[] =
{
    "toString",
    "toDateString",
    "toTimeString",
    "toLocaleString",
    "toLocaleDateString",
    "toLocaleTimeString",
    "valueOf",
    "getTime",
    "getFullYear",
    "getUTCFullYear",
    "getMonth",
    "getUTCMonth",
    "getDate",
    "getUTCDate",
    "getDay",
    "getUTCDay",
    "getHours",
    "getUTCHours",
    "getMinutes",
    "getUTCMinutes",
    "getSeconds",
    "getUTCSeconds",
    "getMilliseconds",
    "getUTCMilliseconds",
    "getTimezoneOffset",
    "setTime",
    "setFullYear",
    "setUTCFullYear",
    "setMonth",
    "setUTCMonth",
    "setDate",
    "setUTCDate",
    "setHours",
    "setUTCHours",
    "setMinutes",
    "setUTCMinutes",
    "setSeconds",
    "setUTCSeconds",
    "setMilliseconds",
    "setUTCMilliseconds",
    "toUTCString",
    "toGMTString",
    "getYear",
    "setYear",
};

//
// DateConstructor Methods
//

class DateConstructorMethod : public Code
{
    enum Method
    {
        Parse,
        Utc,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

public:
    DateConstructorMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);

        switch (method)
        {
        case Parse:
            arguments->add(new Identifier("stringValue"));
            break;
        case Utc:
            arguments->add(new Identifier("year"));
            arguments->add(new Identifier("month"));
            arguments->add(new Identifier("date"));
            arguments->add(new Identifier("hours"));
            arguments->add(new Identifier("minutes"));
            arguments->add(new Identifier("seconds"));
            arguments->add(new Identifier("ms"));
        default:
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~DateConstructorMethod()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Register<Value> value;
        switch (method)
        {
        case Parse:
            value = new NumberValue(parse(getScopeChain()->get("stringValue")->toString()));
            break;
        case Utc:
            value = new NumberValue(timeClip(makeDate()));
            break;
        }
        return CompletionType(CompletionType::Return, value, "");
    }

    const char* name() const
    {
        return names[method];
    }

    static int methodCount()
    {
        return MethodCount;
    }
};

const char* DateConstructorMethod::names[] =
{
    "parse",
    "UTC",
};

//
// Date Constructor
//

class DateConstructor : public Code
{
    ObjectValue*            date;
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // Date.prototype

public:
    DateConstructor(ObjectValue* date) :
        date(date),
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
    {
        ObjectValue* function = static_cast<ObjectValue*>(getGlobal()->get("Function"));

        arguments->add(new Identifier("year"));
        arguments->add(new Identifier("month"));
        arguments->add(new Identifier("date"));
        arguments->add(new Identifier("hours"));
        arguments->add(new Identifier("minutes"));
        arguments->add(new Identifier("seconds"));
        arguments->add(new Identifier("ms"));

        prototype->put("constructor", date);
        prototype->setPrototype(function->getPrototype()->getPrototype());

        for (int i = 0; i < DateMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            DateMethod* method = new DateMethod(function, i);
            function->setCode(method);
            prototype->put(method->name(), function);
        }
        date->setParameterList(arguments);
        date->setScope(getGlobal());
        date->put("prototype", prototype);
        date->setPrototype(function->getPrototype());

        DateValue::prototype = prototype;

        for (int i = 0; i < DateConstructorMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            DateConstructorMethod* method = new DateConstructorMethod(function, i);
            function->setCode(method);
            date->put(method->name(), function);
        }
    }

    ~DateConstructor()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        double tick = NAN;
        if (getScopeChain()->get("year")->isUndefined() ||
            !date->hasInstance(getThis()))
        {
            // Date()
            tick = (DateTime::getNow().getTicks() - Origin.getTicks()) / 10000;
#ifdef __es__
            tick = UTC(tick);
#endif  // __es__
        }
        else if (getScopeChain()->get("month")->isUndefined())
        {
            // Date (value)
            Value* value = getScopeChain()->get("year")->toPrimitive();
            if (value->isString())
            {
                tick = parse(value->toString());
            }
            else
            {
                tick = timeClip(value->toNumber());
            }
        }
        else
        {
            // Date (year, month [, date [, hours [, minutes [, seconds [, ms]]]]])
            tick = timeClip(UTC(makeDate()));
        }

        if (date->hasInstance(getThis()))
        {
            // Date Constructor.
            Register<DateValue> result;
            result = new DateValue(tick);
            ObjectValue* object = static_cast<ObjectValue*>(getThis());
            object->setValueProperty(result);
            return CompletionType(CompletionType::Return, result, "");
        }
        else
        {
            // Date Constructor called as a function.
            CalenderTime time;
            getCalenderTime(static_cast<s64>(tick), &time, TypeLocalTime);
            char buf[MaxDateStringSize];
            getString(&time.t, buf, sizeof(buf), "%c");
            Register<StringValue> s = new StringValue(buf);
            return CompletionType(CompletionType::Return, s, "");
        }
    }
};

ObjectValue* constructDateObject()
{
    ObjectValue* date = new ObjectValue;
    date->setCode(new DateConstructor(date));
    return date;
}
