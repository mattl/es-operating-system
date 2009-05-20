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

#include <algorithm>
#include "esjs.h"
#include "parser.h"
#include "interface.h"

ObjectValue* ArrayValue::prototype;  // Array.prototype

namespace
{
    Value* getElement(Value* a, u32 i)
    {
        char name[12];

        sprintf(name, "%u", i);
        return a->get(name);
    }

    void putElement(Value* a, u32 i, Value* v)
    {
        char name[12];

        sprintf(name, "%u", i);
        ObjectValue* object = dynamic_cast<ObjectValue*>(a);
        if (object)
        {
            // Bypass ArrayValue::put()
            return object->ObjectValue::put(name, v);
        }
        else
        {
            return a->put(name, v);
        }
    }

    bool hasElement(Value* a, u32 i)
    {
        char name[12];

        sprintf(name, "%u", i);
        return a->hasProperty(name);
    }

    bool removeElement(Value* a, u32 i)
    {
        char name[12];

        sprintf(name, "%u", i);
        return a->remove(name);
    }

    u32 getLength(Value* a)
    {
        return a->get("length")->toUint32();
    }

    Value* setLength(Value* a, u32 len)
    {
        Register<NumberValue> length = new NumberValue(len);
        ObjectValue* object = dynamic_cast<ObjectValue*>(a);
        if (object)
        {
            // Bypass ArrayValue::put()
            object->ObjectValue::put("length", length);
        }
        else
        {
            a->put("length", length);
        }
        return length;
    }
}

//
// Array Methods
//

class ArrayMethod : public Code
{
    enum Method
    {
        ToString,
        ToLocaleString,
        Concat,
        Join,
        Pop,
        Push,
        Reverse,
        Shift,
        Slice,
        Sort,
        Splice,
        Unshift,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

    Value* concat(Value* e)
    {
        Register<ArrayValue> a = new ArrayValue;
        int i = 0;
        u32 n = 0;
        ListValue* list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
        do
        {
            char name[12];

            if (dynamic_cast<ArrayValue*>(e))
            {
                u32 len = getLength(e);
                for (u32 k = 0; k < len; ++n, ++k)
                {
                    Value* elem = getElement(e, k);
                    if (!elem->isUndefined())
                    {
                        putElement(a, n, elem);
                    }
                }
            }
            else
            {
                putElement(a, n, e);
                ++n;
            }
            e = (*list)[i];
        } while (i++ < list->length());
        setLength(a, n);
        return a;
    }

    Value* join(Value* value)
    {
        u32 n = getLength(value);
        Value* v = getScopeChain()->get("separator");
        std::string separator = v->isUndefined() ? ", " : v->toString();
        if (n == 0)
        {
            return new StringValue("");
        }
        std::string r;
        for (u32 i = 0; i < n; ++i)
        {
            v = getElement(value, i);
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

    Value* pop(Value* value)
    {
        u32 n = getLength(value);
        if (n == 0)
        {
            setLength(value, n);
            return UndefinedValue::getInstance();
        }
        --n;
        Value* result = getElement(value, n);
        removeElement(value, n);
        setLength(value, n);
        return result;
    }

    Value* push(Value* value)
    {
        u32 n = getLength(value);
        ListValue* list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
        for (u32 i = 0; i < list->length(); ++n, ++i)
        {
            putElement(value, n, (*list)[i]);
        }
        return setLength(value, n);
    }

    Value* reverse(Value* value)
    {
        u32 n = getLength(value);
        u32 end = n / 2;
        for (u32 k = 0; k < end; ++k)
        {
            u32 j = n - k - 1;
            Value* elem = getElement(value, k);
            Value* pair = getElement(value, j);
            if (!pair->isUndefined())
            {
                putElement(value, k, pair);
                if (!elem->isUndefined())
                {
                    putElement(value, j, elem);
                }
                else
                {
                    removeElement(value, j);
                }
            }
            else
            {
                removeElement(value, k);
                if (!elem->isUndefined())
                {
                    putElement(value, j, elem);
                }
                else
                {
                    removeElement(value, j);
                }
            }
        }
        return value;
    }

    Value* shift(Value* value)
    {
        u32 n = getLength(value);
        if (n == 0)
        {
            setLength(value, n);
            return UndefinedValue::getInstance();
        }
        Register<Value> result = value->get("0");
        for (u32 k = 1; k < n; ++k)
        {
            if (hasElement(value, k))
            {
                putElement(value, k - 1, getElement(value, k));
            }
            else
            {
                removeElement(value, k - 1);
            }
        }
        --n;
        removeElement(value, n);
        setLength(value, n);
        return result;
    }

    Value* slice(Value* value)
    {
        double len = getLength(value);

        double start = getScopeChain()->get("start")->toInteger();
        if (start < 0.0)
        {
            start = std::max(len + start, 0.0);
        }
        else
        {
            start = std::min(len, start);
        }
        u32 k = static_cast<u32>(start);

        Value* endValue = getScopeChain()->get("end");
        double end;
        if (endValue->isUndefined())
        {
            end = len;
        }
        else
        {
            end = endValue->toInteger();
            if (end < 0.0)
            {
                end = std::max(0.0, len + end);
            }
            else
            {
                end = std::min(len, end);
            }
        }
        u32 j = static_cast<u32>(end);

        Register<ArrayValue> a = new ArrayValue;
        u32 n;
        for (n = 0; k < j; ++k, ++n)
        {
            if (hasElement(value, k))
            {
                putElement(a, n, getElement(value, k));
            }
        }
        setLength(a, n);
        return a;
    }

    static int sortCompare(Value* x, Value* y)
    {
        if (x->isUndefined() && y->isUndefined())
        {
            return 0;
        }
        else if (x->isUndefined())
        {
            return 1;
        }
        else if (y->isUndefined())
        {
            return -1;
        }

        ObjectValue* function = dynamic_cast<ObjectValue*>(getScopeChain()->get("comparefn"));
        if (function && function->getCode())
        {
            Register<ListValue> newList = new ListValue;
            newList->push(x);
            newList->push(y);
            Value* result = function->call(NullValue::getInstance(), newList);
            return static_cast<int>(result->toNumber());
        }
        else
        {
            return x->toString().compare(y->toString());
        }
    }

    static int cmp(Value* x, Value* y)
    {
        return sortCompare(x, y) < 0;
    }

    Value* sort(Value* value)
    {
        u32 len = getLength(value);
        Value* list[len];

        for (u32 i = 0; i < len; ++i)
        {
            list[i] = getElement(value, i);
        }

        std::sort(list, list + len, cmp);

        for (u32 i = 0; i < len; ++i)
        {
            putElement(value, i, list[i]);
        }

        return value;
    }

    Value* splice(Value* value)
    {
        u32 n = getLength(value);
        double len = n;

        double start = getScopeChain()->get("start")->toInteger();
        if (start < 0.0)
        {
            start = std::max(len + start, 0.0);
        }
        else
        {
            start = std::min(len, start);
        }
        u32 s = static_cast<u32>(start);

        double deleteCount = getScopeChain()->get("deleteCount")->toInteger();
        deleteCount = std::min(std::max(deleteCount, 0.0), len - start);
        u32 d = static_cast<u32>(deleteCount);

        Register<ArrayValue> a = new ArrayValue;
        for (u32 k = 0; k < d; ++k)
        {
            if (hasElement(value, s + k))
            {
                putElement(a, k, getElement(value, s + k));
            }
        }
        setLength(a, d);

        ListValue* list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
        u32 items = std::max(0, list->length() - 2);
        if (items < d)
        {
            for (u32 k = s; k < n - d; ++k)
            {
                if (hasElement(value, k + d))
                {
                    putElement(value, k + items, getElement(value, k + d));
                }
                else
                {
                    removeElement(value, k + items);
                }
            }
            for (u32 k = n; n - d + items < k; --k) // 31
            {
                removeElement(value, k - 1);
            }
        }
        else if (d < items)
        {
            for (u32 k = n - d; k != s; --k)        // 38
            {
                if (hasElement(value, k + d - 1))
                {
                    putElement(value, k + items - 1, getElement(value, k + d - 1));
                }
                else
                {
                    removeElement(value, k + items - 1);
                }
            }
        }

        u32 k = s;
        for (u32 j = 0; j < items; ++j, ++k)        // 48
        {
            putElement(value, k, (*list)[j + 2]);
        }

        setLength(value, n - d + items);            // 53
        return a;
    }

    Value* unshift(Value* value)
    {
        u32 len = getLength(value);
        ListValue* list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
        int items = list->length();
        for (int k = items; 0 < k; --k)
        {
            if (hasElement(value, k - 1))
            {
                putElement(value, k + items - 1, getElement(value, k - 1));
            }
            else
            {
                removeElement(value, k + items - 1);
            }
        }

        for (int k = 0; k < items; ++k)             // 15
        {
            putElement(value, k, (*list)[k]);
        }

        return setLength(value, len + items);       // 21
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
        case Sort:
            arguments->add(new Identifier("comparefn"));
            break;
        case Slice:
            arguments->add(new Identifier("start"));
            arguments->add(new Identifier("end"));
            break;
        case Splice:
            arguments->add(new Identifier("start"));
            arguments->add(new Identifier("deleteCount"));
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
        Register<StringValue> separator;
        switch (method)
        {
        case ToLocaleString:
            separator = new StringValue(",");
            getThis()->put("separator", separator);
            // FALL THROUGH
        case ToString:
            if (!dynamic_cast<ArrayValue*>(getThis()))
            {
                throw getErrorInstance("TypeError");
            }
            value = join(getThis());
            break;
        case Concat:
            value = concat(getThis());
            break;
        case Join:
            value = join(getThis());
            break;
        case Pop:
            value = pop(static_cast<ObjectValue*>(getThis()));
            break;
        case Push:
            value = push(getThis());
            break;
        case Reverse:
            value = reverse(getThis());
            break;
        case Shift:
            value = shift(getThis());
            break;
        case Slice:
            value = slice(getThis());
            break;
        case Sort:
            value = sort(getThis());
            break;
        case Splice:
            value = splice(getThis());
            break;
        case Unshift:
            value = unshift(getThis());
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
    "toLocaleString",
    "concat",
    "join",
    "pop",
    "push",
    "reverse",
    "shift",
    "slice",
    "sort",
    "splice",
    "unshift"
};

//
// Array Constructor
//

class ArrayConstructor : public Code
{
    ObjectValue*            array;
    FormalParameterList*    arguments;
    ArrayValue*             prototype;  // Array.prototype

public:
    ArrayConstructor(ObjectValue* array) :
        array(array),
        arguments(new FormalParameterList),
        prototype(0)
    {
        ObjectValue* function = static_cast<ObjectValue*>(getGlobal()->get("Function"));

        ArrayValue::prototype = static_cast<ObjectValue*>(function->getPrototype()->getPrototype());
        prototype = new ArrayValue;
        prototype->put("constructor", array);

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
        property->putValue(value);
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
