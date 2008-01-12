/*
 * Copyright (c) 2007
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

#include <math.h>
#include "esjs.h"
#include "parser.h"

class MathValue : public ObjectValue
{
public:
    const char* getClass() const
    {
        return "Math";
    }
};

//
// Math Object
//

class MathMethod : public Code
{
    enum Method
    {
        Abs,
        Acos,
        Asin,
        Atan,
        Atan2,
        Ceil,
        Cos,
        Exp,
        Floor,
        Log,
        Max,
        Min,
        Pow,
        Random,
        Round,
        Sin,
        Sqrt,
        Tan,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

public:
    MathMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);
        switch (method)
        {
        case Atan2:
            arguments->add(new Identifier("y"));
            arguments->add(new Identifier("x"));
            break;
        case Max:
        case Min:
            arguments->add(new Identifier("value1"));
            arguments->add(new Identifier("value2"));
            break;
        case Pow:
            arguments->add(new Identifier("x"));
            arguments->add(new Identifier("y"));
            break;
        case Random:
            break;
        default:
            arguments->add(new Identifier("x"));
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~MathMethod()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        double x;
        double y;
        ListValue* list;

        switch (method)
        {
        case Atan2:
            y = getScopeChain()->get("y")->toNumber();
            x = getScopeChain()->get("x")->toNumber();
            x = atan2(y, x);
            break;
        case Max:
            list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
            x = -INFINITY;
            for (int i = 0; i < list->length(); ++i)
            {
                y = (*list)[i]->toNumber();
                if (isnan(y))
                {
                    x = NAN;
                    break;
                }
                if (x < y)
                {
                    x = y;
                }
            }
            break;
        case Min:
            list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
            x = INFINITY;
            for (int i = 0; i < list->length(); ++i)
            {
                y = (*list)[i]->toNumber();
                if (isnan(y))
                {
                    x = NAN;
                    break;
                }
                if (y < x)
                {
                    x = y;
                }
            }
            break;
        case Pow:
            x = getScopeChain()->get("x")->toNumber();
            y = getScopeChain()->get("y")->toNumber();
            x = pow(x, y);
            break;
        case Random:
            x = drand48();
            break;
        default:
            x = getScopeChain()->get("x")->toNumber();
            switch (method)
            {
            case Abs:
                x = fabs(x);
                break;
            case Acos:
                x = acos(x);
                break;
            case Asin:
                x = asin(x);
                break;
            case Atan:
                x = atan(x);
                break;
            case Ceil:
                x = ceil(x);
                break;
            case Cos:
                x = cos(x);
                break;
            case Exp:
                x = exp(x);
                break;
            case Floor:
                x = floor(x);
                break;
            case Log:
                x = log(x);
                break;
            case Round:
                x = round(x);
                break;
            case Sin:
                x = sin(x);
                break;
            case Sqrt:
                x = sqrt(x);
                break;
            case Tan:
                x = tan(x);
                break;
            default:
                break;
            }
            break;
        }
        return CompletionType(CompletionType::Return, new NumberValue(x), "");
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

const char* MathMethod::names[] =
{
    "abs",
    "acos",
    "asin",
    "atan",
    "atan2",
    "ceil",
    "cos",
    "exp",
    "floor",
    "log",
    "max",
    "min",
    "pow",
    "random",
    "round",
    "sin",
    "sqrt",
    "tan",
};

ObjectValue* constructMathObject()
{
    MathValue* math = new MathValue;

    math->put("E", new NumberValue(2.7182818284590452354));
    math->put("LN10", new NumberValue(2.302585092994046));
    math->put("LN2", new NumberValue(0.6931471805599453));
    math->put("LOG2E", new NumberValue(1.4426950408889634));
    math->put("LOG10E", new NumberValue(0.4342944819032518));
    math->put("PI", new NumberValue(3.1415926535897932));
    math->put("SQRT1_2", new NumberValue(0.7071067811865476));
    math->put("SQRT2", new NumberValue(1.4142135623730951));

    for (int i = 0; i < MathMethod::methodCount(); ++i)
    {
        ObjectValue* function = new ObjectValue;
        MathMethod* method = new MathMethod(function, i);
        function->setCode(method);
        math->put(method->name(), function);
    }

    return math;
}
