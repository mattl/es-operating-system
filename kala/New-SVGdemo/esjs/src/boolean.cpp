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
