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

#include "esjs.h"

ExecutionContext* ExecutionContext::context;

namespace
{
    UndefinedValue  undefinedValue;
    NullValue       nullValue;
    BoolValue       trueValue(true);
    BoolValue       falseValue(false);
    Stack           stack;
}

int Value::thresh = 65536;
long long Value::allocCount;
long long Value::freeCount;
Value::List Value::newSet;
Value::List Value::oldSet;
Value::List Value::rememberedSet;

Value* Value::get(const std::string& name)
{
    return &undefinedValue;
}

UndefinedValue* UndefinedValue::getInstance()
{
    return &undefinedValue;
}

NullValue* NullValue::getInstance()
{
    return &nullValue;
}

BoolValue* BoolValue::getInstance(bool value)
{
    return value ? &trueValue : &falseValue;
}

ExecutionContext::ExecutionContext(Value* self, ObjectValue* object, ListValue* list) :
    thisValue(0),
    scopeChain(0),
    link(context)
{
    context = this; // To be marked
    thisValue = self->isObject() ? self : getGlobal();
    scopeChain = new ObjectValue;   // 10.1.6 Activation Object

    list->put("callee", object, ObjectValue::DontEnum);
    scopeChain->put("arguments", list, ObjectValue::DontEnum);
    // 10.1.3 Variable Instantiation
    object->getParameterList()->instantiate(scopeChain, list);
    scopeChain->setNext(object->getScope());
}

ObjectValue* getScopeChain()
{
    return ExecutionContext::getCurrent()->getScopeChain();
}

Value* getThis()
{
    return ExecutionContext::getCurrent()->getThis();
}

Stack* getStack()
{
    return &stack;
}

void Value::sweep(bool full)
{
    if (full)
    {
        Value::List::Iterator i = oldSet.begin();
        while (Value* value = i.next())
        {
            value->refresh();
        }

        Value::List::Iterator j = rememberedSet.begin();
        while (Value* value = j.next())
        {
            value->refresh();
        }
    }

    // Mark
    getStack()->mark();
    ExecutionContext* context = ExecutionContext::getCurrent();
    if (context)
    {
        context->mark();
    }
    Value::List::Iterator i = rememberedSet.begin();
    while (Value* value = i.next())
    {
        value->mark();
    }
    Value::List::Iterator j = rememberedSet.begin();
    while (Value* value = j.next())
    {
        value->clear();
    }

    // Sweep
    Value::List::Iterator k = newSet.begin();
    while (Value* value = k.next())
    {
        if (!value->isMarked())
        {
            delete value;
        }
        else
        {
            value->clear();
        }
    }

    // Check
#ifndef NDEBUG
    Value::List::Iterator l = rememberedSet.begin();
    while (Value* value = l.next())
    {
        ASSERT(!value->isMarked());
    }

    Value::List::Iterator m = oldSet.begin();
    while (Value* value = m.next())
    {
        ASSERT(!value->isMarked());
    }
#endif
}

ObjectValue::~ObjectValue()
{
    if (mortal)
    {
        if (parameterList)
        {
            delete parameterList;
        }

        if (code)
        {
            delete code;
        }
    }
}

Value* ObjectValue::toPrimitive(int hint)
{
    if (hint == Value::UndefinedType)
    {
        hint = Value::NumberType;
    }

    Register<ListValue> list = new ListValue;
    Register<Value> value;
    bool number = (hint == Value::NumberType) ? true : false;
    for (int i = 0; i < 2; ++i, number != number)
    {
        ObjectValue* function = dynamic_cast<ObjectValue*>(get(number ? "valueOf" : "toString"));
        if (function && function->getCode())
        {
            value = function->call(this, list);
            if (value->isPrimitive())
            {
                return value;
            }
        }
    }
    throw getErrorInstance("TypeError");
}

CompletionType ObjectValue::iterate(Expression* expression, Statement* statement)
{
    Register<Value> value;
    Tree<std::string, Property*>::Iterator iter = properties.begin();
    while (Tree<std::string, Property*>::Node* node = iter.next())
    {
        Property* property = node->getValue();
        if (property->dontEnum())
        {
            continue;
        }

        Register<Value> left = expression->evaluate();
        Register<StringValue> key = new StringValue(node->getKey());
        left->putValue(key);

        CompletionType result = statement->evaluate();
        if (result.getValue())
        {
            value = result.getValue();
        }
        if (result.isBreak() && result.getTarget() == "")
        {
            break;
        }
        if (result.isContinue() && result.getTarget() == "")
        {
            continue;
        }
        if (result.isAbrupt())
        {
            return result;
        }
    }
    return CompletionType(CompletionType::Normal, value, "");
}

void ObjectValue::setParameterList(FormalParameterList* list)
{
    parameterList = list;
    Register<NumberValue> length = new NumberValue(parameterList->getLength());
    put("length", length);  // { DontDelete, ReadOnly, DontEnum }
}

// 13.2.1
Value* ObjectValue::call(Value* self, ListValue* list)
{
    // Establish a new execution context using F's FormalParameterList,
    // the passed arguments list, and the this value as described in 10.2.3.
    ExecutionContext* context = new ExecutionContext(self, this, list);

    CompletionType result;
    Value*         value;
    try
    {
        result = code->evaluate();
        value = result.getValue();
    }
    catch (Value* e)
    {
        result.setType(CompletionType::Throw);
        value = e;
    }

    // Exit the execution context established in step 1, restoring the
    // previous execution context.
    delete context;

    if (result.isThrow())
    {
        throw value;
    }
    if (result.isReturn())
    {
        return value;
    }
    ASSERT(result.isNormal());
    return UndefinedValue::getInstance();
}
