/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

#include <string.h>
#include <es/any.h>
#include <es/endian.h>
#include <es/formatter.h>
#include <es/interfaceData.h>
#include <es/object.h>
#include <es/base/IProcess.h>
#include <es/hashtable.h>
#include <es/reflect.h>

// TODO use proper name prefix or namespace
namespace es
{
    Reflect::Interface& getInterface(const char* iid);
    Object* getConstructor(const char* iid);
    extern unsigned char* defaultInterfaceInfo[];
    extern size_t defaultInterfaceCount;
}  // namespace es

#ifndef VERBOSE
#define PRINTF(...)     (__VA_ARGS__)
#else
#define PRINTF(...)     report(__VA_ARGS__)
#endif

class ObjectValue;

#include "interface.h"

extern es::CurrentProcess* System();

namespace
{
    const int GuidStringLength = 37;        // Including terminating zero
}

//
// invoke
//

typedef long long (*InterfaceMethod)(void* self, ...);

static char heap[64*1024];

static Value* invoke(const char* iid, int number, InterfaceMethod** self, ListValue* list)
{
    if (!self)
    {
        throw getErrorInstance("TypeError");
    }

    Reflect::Interface interface = es::getInterface(iid);
    Reflect::Method method(interface.getMethod(number));
    PRINTF("invoke %s.%s(%p)\n", interface.getName().c_str(), method.getName().c_str(), self);

    // Set up parameters
    Any argv[9];
    Any* argp = argv;
    int ext = 0;    // extra parameter count

    // Set this
    *argp++ = Any(reinterpret_cast<intptr_t>(self));

    // In the following implementation, we assume no out nor inout attribute is
    // used for parameters.

    Reflect::Type returnType = method.getReturnType();
    switch (returnType.getType())
    {
    case Reflect::kAny:
        // Any op(void* buf, int len, ...);
        // FALL THROUGH
    case Reflect::kString:
        // const char* op(xxx* buf, int len, ...);
        *argp++ = Any(reinterpret_cast<intptr_t>(heap));
        *argp++ = Any(sizeof(heap));
        break;
    case Reflect::kSequence:
        // int op(xxx* buf, int len, ...);
        *argp++ = Any(reinterpret_cast<intptr_t>(heap));
        ++ext;
        *argp++ = Any(static_cast<int32_t>(((*list)[0])->toNumber()));
        break;
    case Reflect::kArray:
        // void op(xxx[x] buf, ...);
        *argp++ = Any(reinterpret_cast<intptr_t>(heap));
        break;
    }

    Reflect::Parameter param = method.listParameter();
    for (int i = ext; param.next(); ++i, ++argp)
    {
        Reflect::Type type(param.getType());
        Value* value = (*list)[i];
        switch (type.getType())
        {
        case Reflect::kAny:
            // Any variant, ...
            switch (value->getType()) {
            case Value::BoolType:
                *argp = Any(static_cast<bool>(value->toBoolean()));
                break;
            case Value::StringType:
                *argp = Any(value->toString().c_str());
                break;
            case Value::NumberType:
                *argp = Any(static_cast<double>(value->toNumber()));
                break;
            case Value::ObjectType:
                if (InterfacePointerValue* unknown = dynamic_cast<InterfacePointerValue*>(value))
                {
                    *argp = Any(unknown->getObject());
                }
                else
                {
                    // XXX expose ECMAScript object
                    *argp = Any(static_cast<Object*>(0));
                }
                break;
            default:
                *argp = Any();
                break;
            }
            argp->makeVariant();
            break;
        case Reflect::kSequence:
            // xxx* buf, int len, ...
            // XXX Assume sequence<octet> now...
            *argp++ = Any(reinterpret_cast<intptr_t>(value->toString().c_str()));
            value = (*list)[++i];
            *argp = Any(static_cast<int32_t>(value->toNumber()));
            break;
        case Reflect::kString:
            *argp = Any(value->toString().c_str());
            break;
        case Reflect::kArray:
            // void op(xxx[x] buf, ...);
            // XXX expand data
            break;
        case Reflect::kObject:
            if (InterfacePointerValue* unknown = dynamic_cast<InterfacePointerValue*>(value))
            {
                *argp = Any(unknown->getObject());
            }
            else
            {
                *argp = Any(static_cast<Object*>(0));
            }
            break;
        case Reflect::kBoolean:
            *argp = Any(static_cast<bool>(value->toBoolean()));
            break;
        case Reflect::kPointer:
            *argp = Any(static_cast<intptr_t>(value->toNumber()));
            break;
        case Reflect::kShort:
            *argp = Any(static_cast<int16_t>(value->toNumber()));
            break;
        case Reflect::kLong:
            *argp = Any(static_cast<int32_t>(value->toNumber()));
            break;
        case Reflect::kOctet:
            *argp = Any(static_cast<uint8_t>(value->toNumber()));
            break;
        case Reflect::kUnsignedShort:
            *argp = Any(static_cast<uint16_t>(value->toNumber()));
            break;
        case Reflect::kUnsignedLong:
            *argp = Any(static_cast<uint32_t>(value->toNumber()));
            break;
        case Reflect::kLongLong:
            *argp = Any(static_cast<int64_t>(value->toNumber()));
            break;
        case Reflect::kUnsignedLongLong:
            *argp = Any(static_cast<uint64_t>(value->toNumber()));
            break;
        case Reflect::kFloat:
            *argp = Any(static_cast<float>(value->toNumber()));
            break;
        case Reflect::kDouble:
            *argp = Any(static_cast<double>(value->toNumber()));
            break;
        default:
            break;
        }
    }

    // Invoke method
    Register<Value> value;
    unsigned methodNumber = interface.getInheritedMethodCount() + number;
    int argc = argp - argv;
    switch (returnType.getType())
    {
    case Reflect::kAny:
        {
            Any result = apply(argc, argv, (Any (*)()) ((*self)[methodNumber]));
            switch (result.getType())
            {
            case Any::TypeVoid:
                value = NullValue::getInstance();
                break;
            case Any::TypeBool:
                value = BoolValue::getInstance(static_cast<bool>(result));
                break;
            case Any::TypeOctet:
                value = new NumberValue(static_cast<uint8_t>(result));
                break;
            case Any::TypeShort:
                value = new NumberValue(static_cast<int16_t>(result));
                break;
            case Any::TypeUnsignedShort:
                value = new NumberValue(static_cast<uint16_t>(result));
                break;
            case Any::TypeLong:
                value = new NumberValue(static_cast<int32_t>(result));
                break;
            case Any::TypeUnsignedLong:
                value = new NumberValue(static_cast<uint32_t>(result));
                break;
            case Any::TypeLongLong:
                value = new NumberValue(static_cast<int64_t>(result));
                break;
            case Any::TypeUnsignedLongLong:
                value = new NumberValue(static_cast<uint64_t>(result));
                break;
            case Any::TypeFloat:
                value = new NumberValue(static_cast<float>(result));
                break;
            case Any::TypeDouble:
                value = new NumberValue(static_cast<double>(result));
                break;
            case Any::TypeString:
                if (const char* string = static_cast<const char*>(result))
                {
                    value = new StringValue(string);
                }
                else
                {
                    value = NullValue::getInstance();
                }
                break;
            case Any::TypeObject:
                if (Object* unknown = static_cast<Object*>(result))
                {
                    ObjectValue* instance = new InterfacePointerValue(unknown);
                    instance->setPrototype(getGlobal()->get(es::getInterface(Object::iid()).getName())->get("prototype"));   // XXX Should use IID
                    value = instance;
                }
                else
                {
                    value = NullValue::getInstance();
                }
                break;
            default:
                value = NullValue::getInstance();
                break;
            }
        }
        break;
    case Reflect::kBoolean:
        value = BoolValue::getInstance(static_cast<bool>(apply(argc, argv, (bool (*)()) ((*self)[methodNumber]))));
        break;
    case Reflect::kOctet:
        value = new NumberValue(static_cast<uint8_t>(apply(argc, argv, (uint8_t (*)()) ((*self)[methodNumber]))));
        break;
    case Reflect::kShort:
        value = new NumberValue(static_cast<int16_t>(apply(argc, argv, (int16_t (*)()) ((*self)[methodNumber]))));
        break;
    case Reflect::kUnsignedShort:
        value = new NumberValue(static_cast<uint16_t>(apply(argc, argv, (uint16_t (*)()) ((*self)[methodNumber]))));
        break;
    case Reflect::kLong:
        value = new NumberValue(static_cast<int32_t>(apply(argc, argv, (int32_t (*)()) ((*self)[methodNumber]))));
        break;
    case Reflect::kUnsignedLong:
        value = new NumberValue(static_cast<uint32_t>(apply(argc, argv, (uint32_t (*)()) ((*self)[methodNumber]))));
        break;
    case Reflect::kLongLong:
        value = new NumberValue(static_cast<int64_t>(apply(argc, argv, (int64_t (*)()) ((*self)[methodNumber]))));
        break;
    case Reflect::kUnsignedLongLong:
        value = new NumberValue(static_cast<uint64_t>(apply(argc, argv, (uint64_t (*)()) ((*self)[methodNumber]))));
        break;
    case Reflect::kFloat:
        value = new NumberValue(static_cast<float>(apply(argc, argv, (float (*)()) ((*self)[methodNumber]))));
        break;
    case Reflect::kDouble:
        value = new NumberValue(apply(argc, argv, (double (*)()) ((*self)[methodNumber])));
        break;
    case Reflect::kPointer:
        value = new NumberValue(static_cast<intptr_t>(apply(argc, argv, (intptr_t (*)()) ((*self)[methodNumber]))));
        break;
    case Reflect::kString:
        {
            heap[0] = '\0';
            Any result = apply(argc, argv, (const char* (*)()) ((*self)[methodNumber]));
            if (const char* string = static_cast<const char*>(result))
            {
                value = new StringValue(string);
            }
            else
            {
                value = NullValue::getInstance();
            }
        }
        break;
    case Reflect::kSequence:
        {
            // XXX Assume sequence<octet> now...
            int32_t count = apply(argc, argv, (int32_t (*)()) ((*self)[methodNumber]));
            if (count < 0)
            {
                count = 0;
            }
            heap[count] = '\0';
            value = new StringValue(heap);
        }
        break;
    case Reflect::kObject:
        if (Object* unknown = apply(argc, argv, (Object* (*)()) ((*self)[methodNumber])))
        {
            ObjectValue* instance = new InterfacePointerValue(unknown);
            // TODO: check Object and others
            instance->setPrototype(getGlobal()->get(es::getInterface(returnType.getQualifiedName().c_str()).getName())->get("prototype"));   // XXX Should use IID
            value = instance;
        }
        else
        {
            value = NullValue::getInstance();
        }
        break;
    case Reflect::kVoid:
        apply(argc, argv, (int32_t (*)()) ((*self)[methodNumber]));
        value = NullValue::getInstance();
        break;
    }
    return value;
}

static Value* invoke(const char* iid, int number, InterfacePointerValue* object, ListValue* list)
{
    InterfaceMethod** self = reinterpret_cast<InterfaceMethod**>(object->getObject());
    if (!self)
    {
        throw getErrorInstance("TypeError");
    }
    Value* value = invoke(iid, number, self, list);
    if (strcmp(iid, Object::iid()) == 0 && number == 2)   // Object::release()
    {
        object->clearObject();
    }
    return value;
}

//
// AttributeValue
//

class AttributeValue : public ObjectValue
{
    bool        readOnly;
    const char* iid;
    int         getter;     // Method number
    int         setter;     // Method number

public:
    AttributeValue(const char* iid) :
        readOnly(true),
        iid(iid),
        getter(0),
        setter(0)
    {
    }

    ~AttributeValue()
    {
    }

    void addSetter(int number)
    {
        readOnly = false;
        setter = number;
    }

    void addGetter(int number)
    {
        getter = number;
    }

    // Getter
    virtual Value* get(Value* self)
    {
        if (dynamic_cast<InterfacePointerValue*>(self))
        {
            Register<ListValue> list = new ListValue;
            return invoke(iid, getter, static_cast<InterfacePointerValue*>(self), list);
        }
        else
        {
            return this;
        }
    }

    // Setter
    virtual bool put(Value* self, Value* value)
    {
        if (dynamic_cast<InterfacePointerValue*>(self) && !readOnly)
        {
            Register<ListValue> list = new ListValue;
            list->push(value);
            invoke(iid, setter, static_cast<InterfacePointerValue*>(self), list);
        }
        return true;
    }
};

//
// InterfaceMethodCode
//

class InterfaceMethodCode : public Code
{
    FormalParameterList*    arguments;
    ObjectValue*            prototype;

    const char*             iid;
    int                     number;     // Method number

public:
    InterfaceMethodCode(ObjectValue* object, const char* iid, int number) :
        arguments(new FormalParameterList),
        prototype(new ObjectValue),
        iid(iid),
        number(number)
    {
        Reflect::Interface interface = es::getInterface(iid);
        Reflect::Method method(interface.getMethod(number));

#if 0
        // Add as many arguments as required.
        for (int i = 0; i < method.getParameterCount(); ++i)
        {
            Reflect::Parameter param(method.getParameter(i));
            if (param.isInput())
            {
                // Note the name "arguments" is reserved in a ECMAScript function.
                ASSERT(strcmp(param.getName(), "arguments") != 0);
                arguments->add(new Identifier(param.getName()));
            }
        }
#endif

        object->setParameterList(arguments);
        object->setScope(getGlobal());

        // Create Interface.prototype
        prototype->put("constructor", object);
        object->put("prototype", prototype);

    }

    ~InterfaceMethodCode()
    {
        delete arguments;
    }

    CompletionType evaluate()
    {
        InterfacePointerValue* object = dynamic_cast<InterfacePointerValue*>(getThis());
        if (!object)
        {
            throw getErrorInstance("TypeError");
        }
        ListValue* list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
        Register<Value> value = invoke(iid, number, object, list);
        return CompletionType(CompletionType::Return, value, "");
    }
};

//
// AttributeSetterValue
//

class AttributeSetterValue : public ObjectValue
{
    const char* iid;
    int number; // Method number

public:
    AttributeSetterValue(const char* iid, int number) :
        iid(iid),
        number(number)
    {
    }

    ~AttributeSetterValue()
    {
    }

    void put(Value* self, const std::string& name, Value* value)
    {
        Register<ListValue> list = new ListValue;
        Register<StringValue> ident = new StringValue(name);
        list->push(ident);
        list->push(value);
        invoke(iid, number, static_cast<InterfacePointerValue*>(self), list);
        return;
    }
};

//
// AttributeGetterValue
//

class AttributeGetterValue : public ObjectValue
{
    const char* iid;
    int number; // Method number

public:
    AttributeGetterValue(const char* iid, int number) :
        iid(iid),
        number(number)
    {
    }

    ~AttributeGetterValue()
    {
    }

    Value* get(Value* self, const std::string& name)
    {
        Register<ListValue> list = new ListValue;
        Register<StringValue> ident = new StringValue(name);
        list->push(ident);
        return invoke(iid, number, static_cast<InterfacePointerValue*>(self), list);
    }
};

//
// InterfacePrototypeValue
//

class InterfacePrototypeValue : public ObjectValue
{
    enum OpObject {
        IndexGetter,
        IndexSetter,
        NameGetter,
        NameSetter,
        ObjectCount
    };

    ObjectValue* opObjects[ObjectCount];

public:
    InterfacePrototypeValue()
    {
        for (int i = 0; i < ObjectCount; ++i)
        {
            opObjects[i] = 0;
        }
    }

    ~InterfacePrototypeValue()
    {
    }

    void setOpObject(int op, ObjectValue* object)
    {
        ASSERT(op >= 0 && op < ObjectCount);
        opObjects[op] = object;
    }

    ObjectValue* getOpObject(int op)
    {
        ASSERT(op >= 0 && op < ObjectCount);
        return opObjects[op];
    }

    friend class InterfaceConstructor;
    friend class InterfacePointerValue;
};

//
// InterfaceConstructor
//

class InterfaceConstructor : public Code
{
    ObjectValue*             constructor;
    FormalParameterList*     arguments;
    InterfacePrototypeValue* prototype;
    const char*              iid;

public:
    InterfaceConstructor(ObjectValue* object, const char* iid) :
        constructor(object),
        arguments(new FormalParameterList),
        prototype(new InterfacePrototypeValue),
        iid(iid)
    {
        arguments->add(new Identifier("object"));

        object->setParameterList(arguments);
        object->setScope(getGlobal());

        Reflect::Interface interface = es::getInterface(iid);
        // PRINTF("interface: %s\n", interface.getName().c_str());
        for (int i = 0; i < interface.getMethodCount(); ++i)
        {
            // Construct Method object
            Reflect::Method method(interface.getMethod(i));
            if (prototype->hasProperty(method.getName()))
            {
                if (method.isOperation())
                {
                    // XXX Currently overloaded functions are just ignored.
                }
                else
                {
                    AttributeValue* attribute = static_cast<AttributeValue*>(prototype->get(method.getName()));
                    if (method.isGetter())
                    {
                        attribute->addGetter(i);
                    }
                    else
                    {
                        attribute->addSetter(i);
                    }
                }
            }
            else
            {
                if (method.isOperation())
                {
                    ObjectValue* function = new ObjectValue;
                    function->setCode(new InterfaceMethodCode(function, iid, i));
                    prototype->put(method.getName(), function);
#if 0
                    if (method.isIndexGetter())
                    {
                        AttributeGetterValue* getter = new AttributeGetterValue(iid, i);
                        prototype->setOpObject(InterfacePrototypeValue::IndexGetter, getter);
                    }
                    else if (method.isIndexSetter())
                    {
                        AttributeSetterValue* setter = new AttributeSetterValue(iid, i);
                        prototype->setOpObject(InterfacePrototypeValue::IndexSetter, setter);
                    }
                    else if (method.isNameGetter())
                    {
                        AttributeGetterValue* getter = new AttributeGetterValue(iid, i);
                        prototype->setOpObject(InterfacePrototypeValue::NameGetter, getter);
                    }
                    else if (method.isNameSetter())
                    {
                        AttributeSetterValue* setter = new AttributeSetterValue(iid, i);
                        prototype->setOpObject(InterfacePrototypeValue::NameSetter, setter);
                    }
#endif
                }
                else
                {
                    // method is an attribute
                    AttributeValue* attribute = new AttributeValue(iid);
                    if (method.isGetter())
                    {
                        attribute->addGetter(i);
                    }
                    else
                    {
                        attribute->addSetter(i);
                    }
                    prototype->put(method.getName(), attribute);
                }
            }
        }

        if (interface.getQualifiedSuperName() == "")
        {
            prototype->setPrototype(getGlobal()->get("InterfaceStore")->getPrototype()->getPrototype());
        }
        else
        {
            Reflect::Interface super = es::getInterface(interface.getQualifiedSuperName().c_str());
            prototype->setPrototype(getGlobal()->get(super.getName())->get("prototype"));
        }

        // Create Interface.prototype
        prototype->put("constructor", object);
        object->put("prototype", prototype);
    }

    ~InterfaceConstructor()
    {
        delete arguments;
    }

    // Query interface for this interface.
    CompletionType evaluate()
    {
        if (constructor->hasInstance(getThis()))
        {
            // Constructor
            Object* constructor = es::getConstructor(iid);
            if (!constructor)
            {
                throw getErrorInstance("TypeError");
            }
            // TODO: Currently only the default constructor is supported
            std::string ciid = iid;
            ciid += "::Constructor";
            Value* value = invoke(ciid.c_str(), 0, reinterpret_cast<InterfaceMethod**>(constructor), 0);
            return CompletionType(CompletionType::Return, value, "");
        }
        else
        {
            // Cast
            InterfacePointerValue* self = dynamic_cast<InterfacePointerValue*>(getScopeChain()->get("object"));
            if (!self)
            {
                throw getErrorInstance("TypeError");
            }

            Object* object;
            object = self->getObject();
            if (!object || !(object = reinterpret_cast<Object*>(object->queryInterface(iid))))
            {
                // We should throw an error in case called by a new expression.
                throw getErrorInstance("TypeError");
            }

            ObjectValue* value = new InterfacePointerValue(object);
            value->setPrototype(prototype);
            return CompletionType(CompletionType::Return, value, "");
        }
    }
};

//
// InterfaceStoreConstructor
//

class InterfaceStoreConstructor : public Code
{
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // Interface.prototype

public:
    InterfaceStoreConstructor(ObjectValue* object) :
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
    {
        ObjectValue* function = static_cast<ObjectValue*>(getGlobal()->get("Function"));

        arguments->add(new Identifier("iid"));

        object->setParameterList(arguments);
        object->setScope(getGlobal());

        // Create Interface.prototype
        prototype->setPrototype(function->getPrototype()->getPrototype());
        prototype->put("constructor", object);
        object->put("prototype", prototype);
        object->setPrototype(function->getPrototype());
    }
    ~InterfaceStoreConstructor()
    {
        delete arguments;
    }

    CompletionType evaluate()
    {
        Value* value = getScopeChain()->get("iid");
        if (!value->isString())
        {
            throw getErrorInstance("TypeError");
        }

        const char* iid = value->toString().c_str();

        Reflect::Interface interface;
        try
        {
            interface = es::getInterface(iid);
        }
        catch (...)
        {
            throw getErrorInstance("TypeError");
        }

        // Construct Interface Object
        ObjectValue* object = new ObjectValue;
        object->setCode(new InterfaceConstructor(object, iid));
        object->setPrototype(prototype);
        return CompletionType(CompletionType::Return, object, "");
    }
};

static bool isIndexAccessor(const std::string& name)
{
    const char* ptr = name.c_str();
    bool hex = false;
    if (strncasecmp(ptr, "0x", 2) == 0)
    {
        ptr += 2;
        hex = true;
    }
    while(*ptr)
    {
        if (hex)
        {
            if (!isxdigit(*ptr))
            {
                return false;
            }
        }
        else
        {
            if (!isdigit(*ptr))
            {
                return false;
            }
        }
        ptr++;
    }
    return true;
}

Value* InterfacePointerValue::get(const std::string& name)
{
    InterfacePrototypeValue* proto = static_cast<InterfacePrototypeValue*>(prototype);
    AttributeGetterValue* getter;

    if (hasProperty(name))
    {
        return ObjectValue::get(name);
    }
    else if ((getter = static_cast<AttributeGetterValue*>(proto->getOpObject(InterfacePrototypeValue::IndexGetter)))
             && isIndexAccessor(name))
    {
        return getter->get(this, name);
    }
    else if ((getter = static_cast<AttributeGetterValue*>(proto->getOpObject(InterfacePrototypeValue::NameGetter))))
    {
        return getter->get(this, name);
    }
    else {
        return ObjectValue::get(name);
    }
}

void InterfacePointerValue::put(const std::string& name, Value* value, int attributes)
{
    InterfacePrototypeValue* proto = static_cast<InterfacePrototypeValue*>(prototype);
    AttributeSetterValue* setter;

    if (!canPut(name))
    {
        return;
    }
    if (hasProperty(name))
    {
        ObjectValue::put(name, value, attributes);
    }
    else if ((setter = static_cast<AttributeSetterValue*>(proto->getOpObject(InterfacePrototypeValue::IndexSetter)))
             && isIndexAccessor(name))
    {
        setter->put(this, name, value);
    }
    else if ((setter = static_cast<AttributeSetterValue*>(proto->getOpObject(InterfacePrototypeValue::NameSetter))))
    {
        setter->put(this, name, value);
    }
    else
    {
        ObjectValue::put(name, value, attributes);
    }
}

ObjectValue* constructInterfaceObject()
{
    ObjectValue* object = new ObjectValue;
    object->setCode(new InterfaceStoreConstructor(object));
    return object;
}

ObjectValue* constructSystemObject(void* system)
{
    for (es::InterfaceData* data = es::interfaceData; data->iid; ++data)
    {
        // Construct Default Interface Object
        Reflect::Interface interface = es::getInterface(data->iid());
        PRINTF("%s\n", interface.getName().c_str());
        ObjectValue* object = new ObjectValue;
        object->setCode(new InterfaceConstructor(object, interface.getQualifiedName()));
        object->setPrototype(getGlobal()->get("InterfaceStore")->getPrototype());
        getGlobal()->put(interface.getName(), object);
    }

    System()->addRef();
    ObjectValue* object = new InterfacePointerValue(System());
    object->setPrototype(getGlobal()->get("CurrentProcess")->get("prototype"));
    return object;
}
