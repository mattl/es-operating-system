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

#include <float.h>
#include "esjs.h"
#include "parser.h"

//
// Number Methods
//

class NumberMethod : public Code
{
    enum Method
    {
        ToString,
        ToLocaleString,
        ValueOf,
        ToFixed,
        ToExponential,
        ToPrecision,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

    Value* toString(double x)
    {
        Value* radix = getScopeChain()->get("radix");
        double r = radix->isUndefined() ? 10.0 : radix->toInteger();
        if (r < 2.0 || 36.0 < r)
        {
            throw getErrorInstance("RangeError");
        }

        std::string m;
        Formatter fmt(m);
        fmt.setMode(Formatter::Mode::ECMAScript);
        if (r == 10.0)
        {
            fmt.print(x);
        }
        else
        {
            fmt.setBase((int) r);
            fmt.print((int) x);
        }
        return new StringValue(m);
    }

    Value* toFixed(double x)
    {
        Value* fractionDigits = getScopeChain()->get("fractionDigits");
        double f = fractionDigits->isUndefined() ? 0.0 : fractionDigits->toInteger();
        if (f < 0.0 || 20.0 < f)
        {
            throw getErrorInstance("RangeError");
        }

        std::string m;
        Formatter fmt(m);
        fmt.setMode(Formatter::Mode::ECMAScript);
        fmt.fixed();
        fmt.setPrecision((int) f);
        fmt.print(x);

        return new StringValue(m);
    }

    Value* toExponential(double x)
    {
        Value* fractionDigits = getScopeChain()->get("fractionDigits");

        std::string m;
        Formatter fmt(m);
        fmt.setMode(Formatter::Mode::ECMAScript);
        fmt.scientific();
        if (!fractionDigits->isUndefined())
        {
            double f = fractionDigits->toInteger();
            if (f < 0.0 || 20.0 < f)
            {
                throw getErrorInstance("RangeError");
            }
            fmt.setPrecision((int) f);
        }
        fmt.print(x);

        return new StringValue(m);
    }

    Value* toPrecision(double x)
    {
        Value* precision = getScopeChain()->get("precision");
        if (precision->isUndefined())
        {
            return toString(x);
        }
        double p = precision->toInteger();
        if (p < 1.0 || 21.0 < p)
        {
            throw getErrorInstance("RangeError");
        }

        std::string m;
        Formatter fmt(m);
        fmt.setMode(Formatter::Mode::ECMAScript);
        fmt.general();
        fmt.setPrecision((int) p);
        fmt.showBase(true);
        fmt.print(x);

        return new StringValue(m);
    }

public:
    NumberMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);
        switch (method)
        {
        case ToString:
        case ToLocaleString:
            arguments->add(new Identifier("radix"));
            break;
        case ToFixed:
            arguments->add(new Identifier("fractionDigits"));
            break;
        case ToExponential:
            arguments->add(new Identifier("fractionDigits"));
            break;
        case ToPrecision:
            arguments->add(new Identifier("precision"));
            break;
        default:
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~NumberMethod()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        if (!getThis()->isObject())
        {
            throw getErrorInstance("TypeError");
        }
        Register<Value> value = static_cast<ObjectValue*>(getThis())->getValueProperty();
        if (!value->isNumber())
        {
            throw getErrorInstance("TypeError");
        }

        switch (method)
        {
        case ToString:
        case ToLocaleString:
            value = toString(value->toNumber());
            break;
        case ValueOf:
            break;
        case ToFixed:
            value = toFixed(value->toNumber());
            break;
        case ToExponential:
            value = toExponential(value->toNumber());
            break;
        case ToPrecision:
            value = toPrecision(value->toNumber());
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

const char* NumberMethod::names[] =
{
    "toString",
    "toLocaleString",
    "valueOf",
    "toFixed",
    "toExponential",
    "toPrecision",
};

//
// Number Constructor
//

class NumberConstructor : public Code
{
    ObjectValue*            number;
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // Number.prototype

public:
    NumberConstructor(ObjectValue* number) :
        number(number),
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
    {
        ObjectValue* function = static_cast<ObjectValue*>(getGlobal()->get("Function"));

        arguments->add(new Identifier("value"));
        prototype->put("constructor", number);
        prototype->setPrototype(function->getPrototype()->getPrototype());

        for (int i = 0; i < NumberMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            NumberMethod* method = new NumberMethod(function, i);
            function->setCode(method);
            prototype->put(method->name(), function);
        }

        number->setParameterList(arguments);
        number->setScope(getGlobal());
        number->setPrototype(function->getPrototype());

        number->put("prototype", prototype,
                    ObjectValue::DontEnum | ObjectValue::DontDelete | ObjectValue::ReadOnly);
        number->put("MAX_VALUE", new NumberValue(DBL_MAX),
                    ObjectValue::DontEnum | ObjectValue::DontDelete | ObjectValue::ReadOnly);
        number->put("MIN_VALUE", new NumberValue(DBL_MIN),
                    ObjectValue::DontEnum | ObjectValue::DontDelete | ObjectValue::ReadOnly);
        number->put("NaN", new NumberValue(NAN),
                    ObjectValue::DontEnum | ObjectValue::DontDelete | ObjectValue::ReadOnly);
        number->put("NEGATIVE_INFINITY", new NumberValue(-INFINITY),
                    ObjectValue::DontEnum | ObjectValue::DontDelete | ObjectValue::ReadOnly);
        number->put("POSITIVE_INFINITY", new NumberValue(INFINITY),
                    ObjectValue::DontEnum | ObjectValue::DontDelete | ObjectValue::ReadOnly);
    }
    ~NumberConstructor()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Register<Value> value = getScopeChain()->get("value");
        Register<NumberValue> result = new NumberValue(value->isUndefined() ? 0.0 : value->toNumber());
        if (number->hasInstance(getThis()))
        {
            // new Number ( [ value ] )
            ObjectValue* object = static_cast<ObjectValue*>(getThis());
            object->setValueProperty(result);
        }
        return CompletionType(CompletionType::Return, result, "");
    }
};

ObjectValue*
NumberValue::toObject()
{
    // Create a new Number object
    ObjectValue* number = dynamic_cast<ObjectValue*>(getGlobal()->get("Number"));
    if (!number || !number->getCode())
    {
        throw getErrorInstance("TypeError");
    }
    Register<ListValue> list = new ListValue;
    list->push(this);
    return number->construct(list);
};

ObjectValue* constructNumberObject()
{
    ObjectValue* number = new ObjectValue;
    number->setCode(new NumberConstructor(number));
    return number;
}
