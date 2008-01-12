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

#ifndef NINTENDO_ESJS_VALUE_H_INCLUDED
#define NINTENDO_ESJS_VALUE_H_INCLUDED

#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif
#include <math.h>
#include <vector>
#include <string>
#include <es.h>
#include <es/exception.h>
#include <es/list.h>
#include <es/tree.h>

#ifndef NAN
#define NAN         __builtin_nan("")
#endif
#ifndef INFINITY
#define INFINITY    __builtin_inf()
#endif

extern int report(const char* spec, ...);

class Value;
class ObjectValue;
class ListValue;

class Expression;
class Statement;

class FormalParameterList;

extern ObjectValue* getGlobal();
extern Value* getErrorInstance(const char* name, const char* message = 0);

class Value
{
public:
    enum Type
    {
        UndefinedType,
        NullType,
        BoolType,
        StringType,
        NumberType,
        PropertyType,
        ObjectType,
        ReferenceType,
        ListType
    };

private:
    static const int MaxAge = 10;

    Link<Value> link;

    typedef List<Value, &Value::link>   List;

    int         age;
    bool        marking;
    List*       set;

    static int thresh;
    static long long allocCount;
    static long long freeCount;
    static List newSet;
    static List oldSet;
    static List rememberedSet;

public:
    Value() :
        age(0),
        marking(false)
    {
        ++allocCount;
        if (allocCount % thresh == 0)
        {
            sweep();
        }

        set = &newSet;
        set->addLast(this);
    }

    virtual ~Value()
    {
        ++freeCount;
        set->remove(this);
#ifndef NDEBUG
        link.prev = link.next = (Value*) 0xa5a5a5a5;
#endif  // NDEBUG
    }

    virtual enum Type getType() const = 0;

    virtual Value* getPrototype()
    {
        return 0;
    }

    virtual void setPrototype(Value* value)
    {
    }

    virtual bool isPrimitive() const
    {
        return false;
    }

    virtual bool isUndefined() const
    {
        return false;
    }

    virtual bool isNull() const
    {
        return false;
    }

    virtual bool isObject() const
    {
        return false;
    }

    virtual bool isString() const
    {
        return false;
    }

    virtual bool isBoolean() const
    {
        return false;
    }

    virtual bool isNumber() const
    {
        return false;
    }

    virtual bool isReference() const
    {
        return false;
    }

    virtual Value* toPrimitive(int hint = NumberType)
    {
        return this;
    }

    virtual bool toBoolean()
    {
        return false;
    }

    virtual double toNumber()
    {
        return NAN;
    };

    double toInteger()
    {
        double x = toNumber();
        if (isnan(x))
        {
            return 0.0;
        }
        if (x == 0.0 || isinf(x))
        {
            return x;
        }
        if (0 <= x)
        {
            return floor(fabs(x));
        }
        else
        {
            return -floor(fabs(x));
        }
    }

    s32 toInt32()
    {
        double x = toNumber();
        if (isnan(x) || x == 0.0 || isinf(x))
        {
            return 0;
        }
        if (0 <= x)
        {
            x = floor(fabs(x));
        }
        else
        {
            x = -floor(fabs(x));
        }
        x = fmod(x, 4294967296.0);
        if (2147483648.0 <= x)
        {
            x -= 4294967296.0;
        }
        return (s32) x;
    }

    u32 toUint32()
    {
        double x = toNumber();
        if (isnan(x) || x == 0.0 || isinf(x))
        {
            return 0;
        }
        if (0 <= x)
        {
            x = floor(fabs(x));
        }
        else
        {
            x = -floor(fabs(x));
        }
        x = fmod(x, 4294967296.0);
        return (u32) x;
    }

    virtual std::string toString()
    {
        return "";
    }

    virtual ObjectValue* toObject()
    {
        throw getErrorInstance("TypeError");
    };

    //
    // Object Type methods
    //
    virtual Value* get(const std::string& name) const;
    virtual void put(const std::string& name, Value* value, int attributes = 0)
    {
        // Do nothing.
    }
    virtual bool canPut(const std::string&) const
    {
        return false;
    }
    virtual bool hasProperty(const std::string&) const
    {
        return false;
    }

    //
    // Reference Type methods
    //
    virtual Value* getValue()
    {
        return this;
    }

    virtual void putValue(Value* value)
    {
        throw getErrorInstance("ReferenceError");
    }

    //
    // Garbage collection methods
    //
    virtual void mark()
    {
        marking = true;
        if (set == &oldSet)
        {
            set->remove(this);
            set = &rememberedSet;
            set->addLast(this);
        }
    }

    virtual bool hasNew()
    {
        return false;
    }

    virtual void clear()
    {
        marking = false;
        if (MaxAge <= ++age)
        {
            age = MaxAge;
            if (!hasNew())
            {
                promote();
            }
            else if (set != &rememberedSet)
            {
                set->remove(this);
                set = &rememberedSet;
                set->addLast(this);
            }
        }
    }

    bool isMarked()
    {
        return marking;
    }

    // Move to old set
    void promote()
    {
        if (set != &oldSet)
        {
            set->remove(this);
            set = &oldSet;
            set->addLast(this);
        }
    }

    void remember(Value* value)
    {
        if (set == &oldSet && value->set == &newSet)
        {
            set->remove(this);
            set = &rememberedSet;
            set->addLast(this);
        }
    }

    // For full GC
    void refresh()
    {
        age = 0;
        if (set != &newSet)
        {
            set->remove(this);
            set = &newSet;
            set->addLast(this);
        }
    }

    bool isNew()
    {
        return set == &newSet;
    }

    bool isOld()
    {
        return set == &oldSet;
    }

    bool isRemembered()
    {
        return set == &rememberedSet;
    }

    //
    // Debug support
    //
    virtual void print()
    {
        report("%s", toString().c_str());
    }

    static void sweep(bool full = false);
    static long long getAllocCount()
    {
        return allocCount;
    }
    static long long getFreeCount()
    {
        return freeCount;
    }
    static void setThresh(int n)
    {
        thresh = n;
    }
};

class CompletionType
{
public:
    enum Type
    {
        Normal,
        Break,
        Continue,
        Return,
        Throw
    };

private:
    Type        type;
    Value*      value;
    std::string target;

public:
    CompletionType(enum Type type = Normal, Value* value = 0, const std::string& target = "") :
        type(type),
        value(value),
        target(target)
    {
    }

    bool isNormal()
    {
        return type == Normal;
    }
    bool isBreak()
    {
        return type == Break;
    }
    bool isContinue()
    {
        return type == Continue;
    }
    bool isReturn()
    {
        return type == Return;
    }
    bool isThrow()
    {
        return type == Throw;
    }

    bool isAbrupt()
    {
        return type != Normal;
    }

    Type getType() const
    {
        return type;
    }
    void setType(Type type)
    {
        this->type = type;
    }

    Value* getValue() const
    {
        return value;
    }
    void setValue(Value* value)
    {
        this->value = value;
    }

    std::string getTarget() const
    {
        return target;
    }
};

class Stack
{
    static const int StackSize = 4096;

    Value*  stack[StackSize];
    Value** pointer;

public:
    Stack()
    {
        memset(stack, 0, sizeof(stack));
        pointer = stack;
    }

    Value** push(Value* v)
    {
        ASSERT(pointer - stack < StackSize);
        *pointer = v;
        return pointer++;
    }

    Value* pop()
    {
        ASSERT(stack < pointer);
        return *--pointer;
    }

    void mark()
    {
        for (Value** iter = stack; iter < pointer; ++iter)
        {
            Value* value = *iter;
            if (value && value->isNew())
            {
                value->mark();
            }
        }
    }
};

extern Stack* getStack();

template<class V>
class Register
{
    V** pointer;

public:
    Register() :
        pointer(reinterpret_cast<V**>(getStack()->push(0)))
    {
    }

    Register(V* value) :
        pointer(reinterpret_cast<V**>(getStack()->push(value)))
    {
    }

    Register(const Register<V>& r) :
        pointer(reinterpret_cast<V**>(getStack()->push(r)))
    {
    }

    ~Register()
    {
        getStack()->pop();
    }

    Register& operator=(const Register& other)
    {
        *pointer = other.value;
        return *this;
    }

    Register& operator=(V* other)
    {
        *pointer = other;
        return *this;
    }

    V* operator->() const
    {
        return *pointer;
    }

    operator V*() const
    {
        return *pointer;
    }
};

class Code
{
public:
    virtual CompletionType evaluate() = 0;
};

class UndefinedValue : public Value
{
public:
    UndefinedValue()
    {
        promote();
        mark();
    }

    enum Value::Type getType() const
    {
        return Value::UndefinedType;
    }

    bool isPrimitive() const
    {
        return true;
    }

    bool isUndefined() const
    {
        return true;
    }

    std::string toString()
    {
        return "undefined";
    }

    void clear()
    {
    }

    static UndefinedValue* getInstance();
};

class NullValue : public Value
{
public:
    NullValue()
    {
        promote();
        mark();
    }

    enum Value::Type getType() const
    {
        return Value::NullType;
    }

    bool isPrimitive() const
    {
        return true;
    }

    bool isNull() const
    {
        return true;
    }

    double toNumber()
    {
        return 0.0;
    };

    std::string toString()
    {
        return "null";
    }

    void clear()
    {
    }

    static NullValue* getInstance();
};

class BoolValue : public Value
{
    bool    value;
public:
    BoolValue(bool value) :
        value(value)
    {
        promote();
        mark();
    }

    enum Value::Type getType() const
    {
        return Value::BoolType;
    }

    bool isPrimitive() const
    {
        return true;
    }

    bool isBoolean() const
    {
        return true;
    }

    bool toBoolean()
    {
        return value ? true : false;
    }

    double toNumber()
    {
        return value ? 1 : 0;
    };

    std::string toString()
    {
        return value ? "true" : "false";
    }

    void clear()
    {
    }

    static BoolValue* getInstance(bool value);
};

class StringValue : public Value
{
    std::string value;

public:
    StringValue()
    {
    }

    StringValue(const std::string& value) :
        value(value)
    {
    }

    enum Value::Type getType() const
    {
        return Value::StringType;
    }

    bool isPrimitive() const
    {
        return true;
    }

    bool isString() const
    {
        return true;
    }

    bool toBoolean()
    {
        return value.empty() ? false : true;
    }

    double toNumber()
    {
        return toNumber(value);
    };

    std::string toString()
    {
        return value;
    }

    ObjectValue* toObject();

    void concatenate(const std::string& str)
    {
        value += str;
    }

    long length()
    {
        return value.size();
    }

    static double toNumber(const std::string& value)
    {
        const char* ptr = value.c_str();
        while (*ptr && isspace(*ptr))
        {
            ++ptr;
        }
        char* end;
        if (strncasecmp(ptr, "0x", 2) == 0)
        {
            ptr += 2;
            long long number = strtoll(ptr, &end, 16);
            return (end != ptr) ? number : NAN;
        }
        else
        {
            double number = strtod(ptr, &end);
            return (end != ptr) ? number : NAN;
        }
    };
};

class NumberValue : public Value
{
    double  value;
public:
    NumberValue(double value) :
        value(value)
    {
    }

    enum Value::Type getType() const
    {
        return Value::NumberType;
    }

    bool isPrimitive() const
    {
        return true;
    }

    bool isNumber() const
    {
        return true;
    }

    bool toBoolean()
    {
        return (value == 0 || value == NAN) ? false : true;
    }

    double toNumber()
    {
        return value;
    };

    std::string toString()
    {
        std::string s;
        Formatter f(s);
        f.format("%g", value);
        return s;
    }
};

class ObjectValue : public Value
{
public:
    static const int ReadOnly = 1;
    static const int DontEnum = 2;
    static const int DontDelete = 4;
    static const int Internal = 8;

protected:
    class Property : public Value
    {
        Value*  value;
        int     attributes;

    public:
        Property(Value* value, int attributes = 0) :
            value(value),
            attributes(attributes)
        {
            ASSERT(value);
        }

        enum Value::Type getType() const
        {
            return Value::PropertyType;
        }

        bool readOnly() const
        {
            return (attributes & ReadOnly) ? true : false;
        }
        bool dontEnum() const
        {
            return (attributes & DontEnum) ? true : false;
        }
        bool dontDelete() const
        {
            return (attributes & DontDelete) ? true : false;
        }

        bool hasNew()
        {
            return value->isNew();
        }

        void mark()
        {
            if (isMarked())
            {
                return;
            }

            Value::mark();
            value->mark();
        }

        Value* getValue()
        {
            return value;
        }

        void setValue(Value* v)
        {
            value = v;
        }

        friend class ObjectValue;
    };

    mutable Tree<std::string, Property*> properties;

    Value*                  prototype;  // The prototype of this object. Aka "__proto__"
    Value*                  value;      // [[Value]] property.
    ObjectValue*            scope;      // The scope chain in which a Function object is executed.
    ObjectValue*            next;       // The next object in the scope chain.

    bool                    mortal;
    FormalParameterList*    parameterList;
    Code*                   code;       // Aka [[Call]]

public:
    ObjectValue() :
        prototype(NullValue::getInstance()),
        value(0),
        scope(0),
        next(0),
        mortal(false),
        parameterList(0),
        code(0)
    {
    }

    ~ObjectValue();

    enum Value::Type getType() const
    {
        return Value::ObjectType;
    }

    Value* getPrototype()
    {
        return prototype;
    }

    void setPrototype(Value* value)
    {
        ASSERT(value);
        prototype = value;
        remember(prototype);
    }

    Value* getValueProperty()
    {
        return value;
    }

    void setValueProperty(Value* value)
    {
        ASSERT(value);
        this->value = value;
        remember(value);
    }

    bool isObject() const
    {
        return true;
    }

    Value* toPrimitive(int hint = Value::NumberType);

    bool toBoolean()
    {
        return true;
    }

    ObjectValue* toObject()
    {
        return this;
    };

    Value* get(const std::string& name) const
    {
        try
        {
            Property* property = properties.get(name);
            ASSERT(property->value);
            return property->value;
        }
        catch (Exception& e)
        {
            if (prototype->isNull())
            {
                return UndefinedValue::getInstance();
            }
        }
        return prototype->get(name);
    }

    void put(const std::string& name, Value* value, int attributes = 0)
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
            property->setValue(value);
        }
        catch (Exception& e)
        {
            property = new Property(value, attributes);
            properties.add(name, property);

            remember(property);
        }

        property->remember(value);
    }

    bool canPut(const std::string& name) const
    {
        try
        {
            return !properties.get(name)->readOnly();
        }
        catch (Exception& e)
        {
            if (prototype->isNull())
            {
                return true;
            }
        }
        return prototype->canPut(name);
    }

    bool hasProperty(const std::string& name) const
    {
        if (properties.contains(name))
        {
            return true;
        }
        if (prototype->isNull())
        {
            return false;
        }
        return prototype->hasProperty(name);
    }

    bool remove(const std::string& name)    // Aka [[Delete]]
    {
        try
        {
            if (properties.get(name)->dontDelete())
            {
                return false;
            }
            properties.remove(name);
            return true;
        }
        catch (Exception& e)
        {
            return false;
        }
    }

    std::string toString()
    {
        Register<Value> value = toPrimitive(Value::StringType);
        return value->toString();
    }

    //
    // Function Object
    //

    void setMortal()
    {
        mortal = true;
    }

    FormalParameterList* getParameterList() const
    {
        return parameterList;
    }

    void setParameterList(FormalParameterList* list)
    {
        parameterList = list;
    }

    void setCode(Code* body)
    {
        code = body;
    }

    Code* getCode()
    {
        return code;
    }

    Value* call(Value* self, ListValue* list);

    bool hasInstance(Value* value)
    {
        if (!code)
        {
            // Not a function object
            throw getErrorInstance("TypeError");
        }
        if (!value->isObject())
        {
            return false;
        }
        Value* proto = get("prototype");
        if (!proto->isObject())
        {
            throw getErrorInstance("TypeError");
        }
        for (;;)
        {
            value = value->getPrototype();
            if (value->isNull())
            {
                return false;
            }
            if (value == proto)
            {
                return true;
            }
        }
    }

    // 13.2.2 [[Contruct]]
    virtual ObjectValue* construct(ListValue* list)
    {
        if (!getCode())
        {
            throw getErrorInstance("TypeError");
        }

        Register<ObjectValue> object = new ObjectValue;

        Register<Value> prototype = get("prototype");
        if (prototype->isObject())
        {
            object->setPrototype(prototype);
        }
        else
        {
            object->setPrototype(getGlobal()->get("Object")->getPrototype());
        }

        Value* result = call(object, list);
        if (result->isObject())
        {
            return static_cast<ObjectValue*>(result);
        }

        return object;
    }

    //
    // Scope Chain
    //
    ObjectValue* getScope() const
    {
        return scope;
    }
    void setScope(ObjectValue* object)
    {
        scope = object;
        remember(object);
    }
    ObjectValue* getNext() const
    {
        return next;
    }
    void setNext(ObjectValue* object)
    {
        next = object;
        remember(next);
    }

    bool hasNew()
    {
        if (prototype->isNew())
        {
            return true;
        }
        if (value && value->isNew())
        {
            return true;
        }
        if (scope && scope->isNew())
        {
            return true;
        }
        if (next && next->isNew())
        {
            return true;
        }

        Tree<std::string, Property*>::Iterator iter = properties.begin();
        while (Tree<std::string, Property*>::Node* node = iter.next())
        {
            if (node->getValue()->isNew())
            {
                return true;
            }
        }

        return false;
    }

    void mark()
    {
        if (isMarked())
        {
            return;
        }

        Value::mark();

        prototype->mark();
        if (value)
        {
            value->mark();
        }
        if (scope)
        {
            scope->mark();
        }
        if (next)
        {
            next->mark();
        }

        Tree<std::string, Property*>::Iterator iter = properties.begin();
        while (Tree<std::string, Property*>::Node* node = iter.next())
        {
            node->getValue()->mark();
        }
    }

    // for-in statement
    CompletionType iterate(Expression* second, Statement* statement);
};

class ArrayValue : public ObjectValue
{
    static ObjectValue* prototype;  // Array.prototype

public:
    ArrayValue()
    {
        Register<ObjectValue> tmp(this);    // Not to be swept
        setPrototype(prototype);
        Register<NumberValue> length = new NumberValue(0);
        this->ObjectValue::put("length", length,
                               ObjectValue::DontEnum | ObjectValue::DontDelete);
    }

    void put(const std::string& name, Value* value, int attributes = 0);

    friend class ArrayConstructor;
};

class ReferenceValue : public Value
{
    Value*      object;
    std::string name;

public:
    ReferenceValue(Value* object, std::string name) :
        object(object),
        name(name)
    {
    }

    enum Value::Type getType() const
    {
        return Value::ReferenceType;
    }

    bool isReference() const
    {
        return true;
    }

    // 8.7.1
    Value* getValue()
    {
        if (object->isNull())
        {
            throw getErrorInstance("ReferenceError");
        }
        return object->get(name);
    }

    // 8.7.2
    void putValue(Value* value)
    {
        if (object->isNull())
        {
            getGlobal()->put(name, value);
        }
        else
        {
            object->put(name, value);
        }
    }

    Value* getBase()
    {
        return object;
    }

    std::string getPropertyName()
    {
        return name;
    }

    bool hasNew()
    {
        return object->isNew() ? true : false;
    }

    void mark()
    {
        if (isMarked())
        {
            return;
        }

        Value::mark();

        object->mark();
    }
};

class ListValue : public Value
{
    std::vector<Value*> list;
public:
    class Iterator
    {
        std::vector<Value*>::iterator bottom;
        std::vector<Value*>::iterator top;

        Iterator(std::vector<Value*>::iterator bottom,
                 std::vector<Value*>::iterator top) :
            bottom(bottom),
            top(top)
        {
        }

    public:
        Value* next()
        {
            if (bottom == top)
            {
                return 0;
            };
            return *top++;
        }

        friend class ListValue;
    };

    enum Value::Type getType() const
    {
        return Value::ListType;
    }

    void push(Value* value)
    {
        ASSERT(value);
        list.push_back(value);

        remember(value);
    }

    void pop()
    {
        list.pop_back();
    }

    int length()
    {
        return list.size();
    }

    Value* operator[](const size_t index)
    {
        if (index < list.size())
        {
            return list[index];
        }
        else
        {
            return UndefinedValue::getInstance();
        }
    }

    Iterator begin()
    {
        return Iterator(list.end(), list.begin());
    }

    virtual void print()
    {
        report("list");
    }

    bool hasNew()
    {
        Iterator iter = begin();
        while (Value* value = iter.next())
        {
            if (value->isNew())
            {
                ASSERT(!isOld());
                return true;
            }
        }
        return false;
    }

    void mark()
    {
        if (isMarked())
        {
            return;
        }

        Value::mark();

        Iterator iter = begin();
        while (Value* value = iter.next())
        {
            value->mark();
        }
    }
};

class ExecutionContext
{
    Value*              thisValue;
    ObjectValue*        scopeChain;
    ExecutionContext*   link;

    static ExecutionContext* context;   // XXX multi-thread

public:
    ExecutionContext(Value* self, ObjectValue* object, ListValue* list);

    ExecutionContext(ObjectValue* activation) :
        scopeChain(activation),
        thisValue(context->thisValue),
        link(context)
    {
        activation->setNext(context->scopeChain);
        context = this;
    }

    ExecutionContext(Value* thisValue, ObjectValue* scopeChain) :
        thisValue(thisValue),
        scopeChain(scopeChain),
        link(0)
    {
        context = this;
    }

    ~ExecutionContext()
    {
        while (this != context)
        {
            delete context;
        }
        context = link;
    }

    ObjectValue* getScopeChain() const
    {
        return scopeChain;
    }

    Value* getThis() const
    {
        return thisValue;
    }

    void mark()
    {
        if (thisValue && thisValue->isNew())
        {
            thisValue->mark();
        }
        if (scopeChain && scopeChain->isNew())
        {
            scopeChain->mark();
        }
        if (link)
        {
            link->mark();
        }
    }

    static ExecutionContext* getCurrent()
    {
        return context;
    }
};

extern ObjectValue* getScopeChain();
extern Value* getThis();

extern ObjectValue* constructStringObject();
extern ObjectValue* constructArrayObject();
extern ObjectValue* constructMathObject();
extern ObjectValue* constructRegExpObject();

#endif  // NINTENDO_ESJS_VALUE_H_INCLUDED
