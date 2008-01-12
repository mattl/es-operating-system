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

#include "esjs.h"
#include "parser.h"
#include "interface.h"

extern Value* stringMatch();
extern Value* stringReplace();
extern Value* stringSearch();

//
// String Methods
//

class StringMethod : public Code
{
    enum Method
    {
        ToString,
        ValueOf,
        Match,
        Replace,
        Search,
        Slice,
        Substring,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

    Value* slice(Value* value)
    {
        std::string s = value->toString();
        double len = s.length();
        double start = getScopeChain()->get("start")->toInteger();
        Value* endValue = getScopeChain()->get("end");
        double end = endValue->isUndefined() ? len : endValue->toInteger();
        if (start < 0)
        {
            start = std::max(start + len, 0.0);
        }
        else
        {
            start = std::min(start, len);
        }
        if (end < 0)
        {
            end = std::max(end + len, 0.0);
        }
        else
        {
            end = std::min(end, len);
        }
        len = std::max(end - start, 0.0);
        s = s.substr((int) start, (int) len);
        return new StringValue(s);
    }

    Value* substring(Value* value)
    {
        std::string s = value->toString();
        double len = s.length();
        double start = getScopeChain()->get("start")->toInteger();
        Value* endValue = getScopeChain()->get("end");
        double end = endValue->isUndefined() ? len : endValue->toInteger();
        start = std::min(std::max(start, 0.0), len);
        end = std::min(std::max(end, 0.0), len);
        if (end < start)
        {
            std::swap(start, end);
        }
        s = s.substr((int) start, (int) (end - start));
        return new StringValue(s);
    }

public:
    StringMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);
        switch (method)
        {
        case Match:
        case Search:
            arguments->add(new Identifier("regexp"));
            break;
        case Replace:
            arguments->add(new Identifier("searchValue"));
            arguments->add(new Identifier("replaceValue"));
            break;
        case Slice:
        case Substring:
            arguments->add(new Identifier("start"));
            arguments->add(new Identifier("end"));
            break;
        default:
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~StringMethod()
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
        case ToString:
        case ValueOf:
            if (!getThis()->isObject())
            {
                throw getErrorInstance("TypeError");
            }
            value = static_cast<ObjectValue*>(getThis())->getValueProperty();
            if (!value->isString())
            {
                throw getErrorInstance("TypeError");
            }
            break;
        case Match:
            value = stringMatch();
            break;
        case Replace:
            value = stringReplace();
            break;
        case Search:
            value = stringSearch();
            break;
        case Slice:
            value = slice(getThis());
            break;
        case Substring:
            value = substring(getThis());
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

const char* StringMethod::names[] =
{
    "toString",
    "valueOf",
    "match",
    "replace",
    "search",
    "slice",
    "substring",
};

//
// String Constructor
//

class StringConstructor : public Code
{
    ObjectValue*            string;
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // String.prototype

public:
    StringConstructor(ObjectValue* string) :
        string(string),
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
    {
        ObjectValue* function = static_cast<ObjectValue*>(getGlobal()->get("Function"));

        arguments->add(new Identifier("value"));
        prototype->put("constructor", string);
        prototype->setPrototype(function->getPrototype()->getPrototype());

        for (int i = 0; i < StringMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            StringMethod* method = new StringMethod(function, i);
            function->setCode(method);
            prototype->put(method->name(), function);
        }

        string->setParameterList(arguments);
        string->setScope(getGlobal());
        string->put("prototype", prototype);
        string->setPrototype(function->getPrototype());
    }
    ~StringConstructor()
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
        Register<StringValue> result = new StringValue(value->isUndefined() ? "" : value->toString());

        if (string->hasInstance(getThis()))
        {
            // Constructor
            ObjectValue* object = static_cast<ObjectValue*>(getThis());
            object->setValueProperty(result);
            Register<NumberValue> length = new NumberValue(result->length());
            object->put("length", length,
                        ObjectValue::DontEnum | ObjectValue::DontDelete | ObjectValue::ReadOnly);
        }

        return CompletionType(CompletionType::Return, result, "");
    }
};

ObjectValue*
StringValue::toObject()
{
    // Create a new String object
    ObjectValue* string = dynamic_cast<ObjectValue*>(getGlobal()->get("String"));
    if (!string || !string->getCode())
    {
        throw getErrorInstance("TypeError");
    }
    Register<ListValue> list = new ListValue;
    list->push(this);
    return string->construct(list);
};

ObjectValue* constructStringObject()
{
    ObjectValue* string = new ObjectValue;
    string->setCode(new StringConstructor(string));
    return string;
}
