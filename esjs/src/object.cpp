/*
 * Copyright (c) 2006, 2007
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

#include <es/handle.h>
#include <es/base/IFile.h>
#include <es/base/IProcess.h>
#include <es/naming/IContext.h>
#include "esjs.h"

//
// Object Methods
//

class ObjectMethod : public Code
{
    enum Method
    {
        ToString,
        ToLocaleString,
        ValueOf,
        HasOwnProperty,
        IsPrototypeOf,
        PropertyIsEnumerable,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

public:
    ObjectMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);
        switch (method)
        {
        case HasOwnProperty:
        case IsPrototypeOf:
        case PropertyIsEnumerable:
            arguments->add(new Identifier("V"));
            break;
        default:
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~ObjectMethod()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Value* object = getThis();
        Value* value;
        std::string s;
        bool result = false;

        switch (method)
        {
        case ToString:
        case ToLocaleString:
            s = "[object ";
            s += object->getClass();
            s += "]";
            value = new StringValue(s);
            break;
        case ValueOf:
            value = object;
            break;
        case HasOwnProperty:
            s = getScopeChain()->get("V")->toString();
            value = BoolValue::getInstance(object->hasOwnProperty(s));
            break;
        case IsPrototypeOf:
            value = getScopeChain()->get("V");
            if (value->isObject())
            {
                for (;;)
                {
                    value = value->getPrototype();
                    if (value->isNull())
                    {
                        break;
                    }
                    else if (value == object)
                    {
                        result = true;
                        break;
                    }
                }
            }
            value = BoolValue::getInstance(result);
            break;
        case PropertyIsEnumerable:
            s = getScopeChain()->get("V")->toString();
            value = BoolValue::getInstance(object->propertyIsEnumerable(s));
            break;
        default:
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

const char* ObjectMethod::names[] =
{
    "toString",
    "toLocaleString",
    "valueOf",
    "hasOwnProperty",
    "isPrototypeOf",
    "propertyIsEnumerable",
};

//
// Object Constructor
//

class ObjectConstructor : public Code
{
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // Object.prototype

public:
    ObjectConstructor(ObjectValue* object) :
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
    {
        ObjectValue* function = static_cast<ObjectValue*>(getGlobal()->get("Function"));

        arguments->add(new Identifier("value"));
        prototype->put("constructor", object);

        for (int i = 0; i < ObjectMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            ObjectMethod* method = new ObjectMethod(function, i);
            function->setCode(method);
            prototype->put(method->name(), function);
        }

        object->setParameterList(arguments);
        object->setScope(getGlobal());
        object->put("prototype", prototype);
        object->setPrototype(function->getPrototype());
    }
    ~ObjectConstructor()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Value* value = getScopeChain()->get("value");
        ObjectValue* object;
        if (value->isUndefined() || value->isNull())
        {
            object = new ObjectValue;
            object->setPrototype(prototype);
        }
        else
        {
            object = value->toObject();
        }
        return CompletionType(CompletionType::Return, object, "");
    }
};

ObjectValue* constructObjectConstructor()
{
    ObjectValue* object = new ObjectValue;
    object->setCode(new ObjectConstructor(object));
    return object;
}
