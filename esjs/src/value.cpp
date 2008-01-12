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

Value* Value::get(const std::string& name) const
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

    scopeChain->put("arguments", list);   // with { DontDelete }
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
