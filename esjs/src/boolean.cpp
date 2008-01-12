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

#include <es/utf.h>
#include "esjs.h"
#include "parser.h"
#include "interface.h"

//
// Boolean Methods
//

class BooleanMethod : public Code
{
    enum Method
    {
        ToString,
        ValueOf,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

public:
    BooleanMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);
        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~BooleanMethod()
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
        if (!value->isBoolean())
        {
            throw getErrorInstance("TypeError");
        }
        switch (method)
        {
        case ToString:
            value = new StringValue(value->toString());
            break;
        case ValueOf:
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

const char* BooleanMethod::names[] =
{
    "toString",
    "valueOf",
};

//
// Boolean Constructor
//

class BooleanConstructor : public Code
{
    ObjectValue*            boolean;
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // Boolean.prototype

public:
    BooleanConstructor(ObjectValue* boolean) :
        boolean(boolean),
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
    {
        ObjectValue* function = static_cast<ObjectValue*>(getGlobal()->get("Function"));

        arguments->add(new Identifier("value"));
        prototype->put("constructor", boolean);
        prototype->setPrototype(function->getPrototype()->getPrototype());

        for (int i = 0; i < BooleanMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            BooleanMethod* method = new BooleanMethod(function, i);
            function->setCode(method);
            prototype->put(method->name(), function);
        }

        boolean->setParameterList(arguments);
        boolean->setScope(getGlobal());
        boolean->put("prototype", prototype);
        boolean->setPrototype(function->getPrototype());
    }
    ~BooleanConstructor()
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
        Register<BoolValue> result = BoolValue::getInstance(value->toBoolean());

        if (boolean->hasInstance(getThis()))
        {
            // Constructor
            ObjectValue* object = static_cast<ObjectValue*>(getThis());
            object->setValueProperty(result);
        }

        return CompletionType(CompletionType::Return, result, "");
    }
};

ObjectValue*
BoolValue::toObject()
{
    // Create a new Boolean object
    ObjectValue* boolean = dynamic_cast<ObjectValue*>(getGlobal()->get("Boolean"));
    if (!boolean || !boolean->getCode())
    {
        throw getErrorInstance("TypeError");
    }
    Register<ListValue> list = new ListValue;
    list->push(this);
    return boolean->construct(list);
};

ObjectValue* constructBooleanObject()
{
    ObjectValue* boolean = new ObjectValue;
    boolean->setCode(new BooleanConstructor(boolean));
    return boolean;
}
