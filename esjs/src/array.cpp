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
#include "interface.h"

ObjectValue* ArrayValue::prototype;  // Array.prototype

//
// Array Methods
//

class ArrayMethod : public Code
{
    enum Method
    {
        ToString,
        Join,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

    Value* join(Value* value)
    {
        u32 length = value->get("length")->toUint32();
        Value* v = getScopeChain()->get("separator");
        std::string separator = v->isUndefined() ? ", " : v->toString();
        if (length == 0)
        {
            return new StringValue("");
        }
        std::string r;
        for (u32 i = 0; i < length; ++i)
        {
            char name[12];
            sprintf(name, "%d", i);
            v = value->get(name);
            if (0 < i)
            {
                r += separator;
            }
            if (!v->isUndefined() && !v->isNull())
            {
                r += v->toString();
            }
        }
        return new StringValue(r);
    }

public:
    ArrayMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);
        switch (method)
        {
        case Join:
            arguments->add(new Identifier("separator"));
            break;
        default:
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~ArrayMethod()
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
            if (!dynamic_cast<ArrayValue*>(getThis()))
            {
                throw getErrorInstance("TypeError");
            }
            value = join(getThis());
            break;
        case Join:
            value = join(getThis());
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

const char* ArrayMethod::names[] =
{
    "toString",
    "join"
};

//
// Array Constructor
//

class ArrayConstructor : public Code
{
    ObjectValue*            array;
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // Array.prototype

public:
    ArrayConstructor(ObjectValue* array) :
        array(array),
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
    {
        ObjectValue* function = static_cast<ObjectValue*>(getGlobal()->get("Function"));

        prototype->put("constructor", array);
        prototype->setPrototype(function->getPrototype()->getPrototype());

        for (int i = 0; i < ArrayMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            ArrayMethod* method = new ArrayMethod(function, i);
            function->setCode(method);
            prototype->put(method->name(), function);
        }

        array->setParameterList(arguments);
        array->setScope(getGlobal());
        array->put("prototype", prototype);
        array->setPrototype(function->getPrototype());

        ArrayValue::prototype = prototype;
    }
    ~ArrayConstructor()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Register<ArrayValue> object = new ArrayValue;

        ListValue* list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
        u32 size;
        if (list->length() == 1)
        {
            Value* len = (*list)[0];
            if (!len->isNumber())
            {
                size = 1;
                object->put("0", len);
            }
            else
            {
                size = (u32) len->toNumber();
                if (size != len->toNumber())
                {
                    throw getErrorInstance("RangeError");
                }
            }
        }
        else
        {
            size = list->length();
            for (u32 i = 0; i < size; ++i)
            {
                char name[12];
                sprintf(name, "%d", i);
                object->put(name, (*list)[i]);
            }
        }

        return CompletionType(CompletionType::Return, object, "");
    }
};

ObjectValue* constructArrayObject()
{
    ObjectValue* array = new ObjectValue;
    array->setCode(new ArrayConstructor(array));
    return array;
}

void ArrayValue::put(const std::string& name, Value* value, int attributes)
{
    ASSERT(value);
    if (!canPut(name))
    {
        return;
    }
    Property* property;
    try
    {
        property = properties.get(name);
        if (name == "length")
        {
            double v = value->toNumber();
            u32 size = value->toUint32();
            if (isnan(v) || size != (u32) v)
            {
                throw getErrorInstance("RangeError");
            }

            u32 len = get("length")->toUint32();
            for (u32 k = size; k < len; ++k)
            {
                char name[12];
                sprintf(name, "%d", k);
                remove(name);
            }
        }
        property->setValue(value);
    }
    catch (Exception& e)
    {
        double v = StringValue::toNumber(name);
        u32 index = (u32) v;
        if (!isnan(v) && index == (u32) v)
        {
            Register<Value> length = get("length");
            u32 len = length->toUint32();
            if (len <= index)
            {
                length = new NumberValue(index + 1);
                ObjectValue::put("length", length,
                                 ObjectValue::DontEnum | ObjectValue::DontDelete);
            }
        }

        property = new Property(value);
        properties.add(name, property);

        remember(property);
    }

    property->remember(value);
}
