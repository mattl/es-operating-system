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
#include <es/uuid.h>
#include <es/base/IInterface.h>
#include <es/base/IProcess.h>
#include <es/hashtable.h>
#include <es/reflect.h>

// TODO use proper name prefix or namespace
namespace es
{
    Reflect::Interface& getInterface(const char* iid);
    es::Interface* getConstructor(const char* iid);
    extern unsigned char* defaultInterfaceInfo[];
    extern size_t defaultInterfaceCount;
}  // namespace es

// #define VERBOSE

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

bool parseGuid(const char* str, Guid* u)
{
    int x;
    int b;
    u8* p = (u8*) u;
    int i;
    int j;

    // 0         1         2         3
    // 012345678901234567890123456789012345
    // 2772cebc-0e69-4582-ae3c-c151bf5a9a55
    for (i = 0, j = 0; i < GuidStringLength - 1; ++i)
    {
        switch (i)
        {
          case 8: case 13: case 18: case 23:
            if (str[i] != '-')
            {
                return false;
            }
            continue;
            break;
          default:
            break;
        }

        x = str[i];
        if (!isxdigit(x))
        {
            return false;
        }
        if (isdigit(x))
        {
            x -= '0';
        }
        else
        {
            x = tolower(x);
            x -= 'a';
            x += 10;
        }

        if (j & 1)
        {
            b = (b << 4) | x;
            *p++ = (u8) b;
        }
        else
        {
            b = x;
        }
        ++j;
        ASSERT(b < 256);
    }
    ASSERT(j == 32);

    u->Data1 = ntohl(u->Data1);
    u->Data2 = ntohs(u->Data2);
    u->Data3 = ntohs(u->Data3);

    return true;
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
    printf("invoke %s.%s(%p)\n", interface.getName(), method.getName(), self);

    // Set up parameters
    Any argv[9];
    Any* argp = argv;
    Guid iidv[9];
    Guid* iidp = iidv;
    int ext = 0;    // extra parameter count
    const char* riid = es::Interface::iid();

    // Set this
    *argp++ = Any(reinterpret_cast<intptr_t>(self));

    // In the following implementation, we assume no out nor inout attribute is
    // used for parameters.

    Reflect::Type returnType = method.getReturnType();
    switch (returnType.getType())
    {
    case Ent::SpecVariant:
        // Any op(void* buf, int len, ...);
        // FALL THROUGH
    case Ent::SpecString:
        // const char* op(xxx* buf, int len, ...);
        *argp++ = Any(reinterpret_cast<intptr_t>(heap));
        *argp++ = Any(sizeof(heap));
        break;
    case Ent::TypeSequence:
        // int op(xxx* buf, int len, ...);
        *argp++ = Any(reinterpret_cast<intptr_t>(heap));
        ++ext;
        *argp++ = Any(static_cast<int32_t>(((*list)[0])->toNumber()));
        break;
    case Ent::SpecUuid:
    case Ent::TypeStructure:
        // void op(struct* buf, ...);
        *argp++ = Any(reinterpret_cast<intptr_t>(heap));
        break;
    case Ent::TypeArray:
        // void op(xxx[x] buf, ...);
        *argp++ = Any(reinterpret_cast<intptr_t>(heap));
        break;
    }

    for (int i = ext; i < ext + method.getParameterCount(); ++i, ++argp)
    {
        Reflect::Parameter param(method.getParameter(i));
        Reflect::Type type(param.getType());
        assert(param.isInput());

        Value* value = (*list)[i];

        switch (type.getType())
        {
        case Ent::SpecVariant:
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
                    *argp = Any(static_cast<es::Interface*>(0));
                }
                break;
            default:
                *argp = Any();
                break;
            }
            argp->makeVariant();
            break;
        case Ent::TypeSequence:
            // xxx* buf, int len, ...
            // XXX Assume sequence<octet> now...
            *argp++ = Any(reinterpret_cast<intptr_t>(value->toString().c_str()));
            value = (*list)[++i];
            *argp = Any(static_cast<int32_t>(value->toNumber()));
            break;
        case Ent::SpecString:
            *argp = Any(value->toString().c_str());
            break;
        case Ent::SpecUuid:
            if (!parseGuid(value->toString().c_str(), iidp))
            {
                throw getErrorInstance("TypeError");
            }
            *argp = Any(iidp);
            // riid = *iidp; TODO: Fix this line later
            ++iidp;
            break;
        case Ent::TypeStructure:
            // void op(struct* buf, ...);
            // XXX expand data
            break;
        case Ent::TypeArray:
            // void op(xxx[x] buf, ...);
            // XXX expand data
            break;
        case Ent::SpecObject:
        case Ent::TypeInterface:
            if (InterfacePointerValue* unknown = dynamic_cast<InterfacePointerValue*>(value))
            {
                *argp = Any(unknown->getObject());
            }
            else
            {
                *argp = Any(static_cast<es::Interface*>(0));
            }
            break;
        case Ent::SpecBool:
            *argp = Any(static_cast<bool>(value->toBoolean()));
            break;
        case Ent::SpecAny:
            *argp = Any(static_cast<intptr_t>(value->toNumber()));
            break;
        case Ent::SpecS16:
            *argp = Any(static_cast<int16_t>(value->toNumber()));
            break;
        case Ent::SpecS32:
            *argp = Any(static_cast<int32_t>(value->toNumber()));
            break;
        case Ent::SpecS8:
        case Ent::SpecU8:
            *argp = Any(static_cast<uint8_t>(value->toNumber()));
            break;
        case Ent::SpecU16:
            *argp = Any(static_cast<uint16_t>(value->toNumber()));
            break;
        case Ent::SpecU32:
            *argp = Any(static_cast<uint32_t>(value->toNumber()));
            break;
        case Ent::SpecS64:
            *argp = Any(static_cast<int64_t>(value->toNumber()));
            break;
        case Ent::SpecU64:
            *argp = Any(static_cast<uint64_t>(value->toNumber()));
            break;
        case Ent::SpecF32:
            *argp = Any(static_cast<float>(value->toNumber()));
            break;
        case Ent::SpecF64:
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
    case Ent::SpecVariant:
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
                if (es::Interface* unknown = static_cast<es::Interface*>(result))
                {
                    ObjectValue* instance = new InterfacePointerValue(unknown);
                    instance->setPrototype(getGlobal()->get(es::getInterface(riid).getName())->get("prototype"));   // XXX Should use IID
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
    case Ent::SpecBool:
        value = BoolValue::getInstance(static_cast<bool>(apply(argc, argv, (bool (*)()) ((*self)[methodNumber]))));
        break;
    case Ent::SpecChar:
    case Ent::SpecS8:
    case Ent::SpecU8:
        value = new NumberValue(static_cast<uint8_t>(apply(argc, argv, (uint8_t (*)()) ((*self)[methodNumber]))));
        break;
    case Ent::SpecWChar:
    case Ent::SpecS16:
        value = new NumberValue(static_cast<int16_t>(apply(argc, argv, (int16_t (*)()) ((*self)[methodNumber]))));
        break;
    case Ent::SpecU16:
        value = new NumberValue(static_cast<uint16_t>(apply(argc, argv, (uint16_t (*)()) ((*self)[methodNumber]))));
        break;
    case Ent::SpecS32:
        value = new NumberValue(static_cast<int32_t>(apply(argc, argv, (int32_t (*)()) ((*self)[methodNumber]))));
        break;
    case Ent::SpecU32:
        value = new NumberValue(static_cast<uint32_t>(apply(argc, argv, (uint32_t (*)()) ((*self)[methodNumber]))));
        break;
    case Ent::SpecS64:
        value = new NumberValue(static_cast<int64_t>(apply(argc, argv, (int64_t (*)()) ((*self)[methodNumber]))));
        break;
    case Ent::SpecU64:
        value = new NumberValue(static_cast<uint64_t>(apply(argc, argv, (uint64_t (*)()) ((*self)[methodNumber]))));
        break;
    case Ent::SpecF32:
        value = new NumberValue(static_cast<float>(apply(argc, argv, (float (*)()) ((*self)[methodNumber]))));
        break;
    case Ent::SpecF64:
        value = new NumberValue(apply(argc, argv, (double (*)()) ((*self)[methodNumber])));
        break;
    case Ent::SpecAny:
        value = new NumberValue(static_cast<intptr_t>(apply(argc, argv, (intptr_t (*)()) ((*self)[methodNumber]))));
        break;
    case Ent::SpecString:
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
    case Ent::TypeSequence:
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
    case Ent::TypeInterface:
        riid = returnType.getInterface().getFullyQualifiedName();
        // FALL THROUGH
    case Ent::SpecObject:
        if (es::Interface* unknown = apply(argc, argv, (es::Interface* (*)()) ((*self)[methodNumber])))
        {
            ObjectValue* instance = new InterfacePointerValue(unknown);
            instance->setPrototype(getGlobal()->get(es::getInterface(riid).getName())->get("prototype"));   // XXX Should use IID
            value = instance;
        }
        else
        {
            value = NullValue::getInstance();
        }
        break;
    case Ent::SpecVoid:
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
    if (strcmp(iid, es::Interface::iid()) == 0 && number == 2)   // es::Interface::release()
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
        PRINTF("interface: %s\n", interface.getName());
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

        if (interface.getFullyQualifiedSuperName() == 0)
        {
            prototype->setPrototype(getGlobal()->get("InterfaceStore")->getPrototype()->getPrototype());
        }
        else
        {
            Reflect::Interface super = es::getInterface(interface.getFullyQualifiedSuperName());
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
            es::Interface* constructor = es::getConstructor(iid);
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

            es::Interface* object;
            object = self->getObject();
            if (!object || !(object = reinterpret_cast<es::Interface*>(object->queryInterface(iid))))
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

static void constructSystemObject(Reflect::Module& module)
{
    for (int i = 0; i < module.getInterfaceCount(); ++i)
    {
        Reflect::Interface interface = module.getInterface(i);

        // Construct Default Interface Object
        printf("%s\n", interface.getName());
        ObjectValue* object = new ObjectValue;
        object->setCode(new InterfaceConstructor(object, interface.getFullyQualifiedName()));
        object->setPrototype(getGlobal()->get("InterfaceStore")->getPrototype());
        getGlobal()->put(interface.getName(), object);
    }

    for (int i = 0; i < module.getModuleCount(); ++i)
    {
        Reflect::Module m(module.getModule(i));
        constructSystemObject(m);
    }
}

ObjectValue* constructSystemObject(void* system)
{
    for (unsigned i = 0; i < es::defaultInterfaceCount; ++i)
    {
        Reflect r(es::defaultInterfaceInfo[i]);
        Reflect::Module global(r.getGlobalModule());
        constructSystemObject(global);
    }

    System()->addRef();
    ObjectValue* object = new InterfacePointerValue(System());
    object->setPrototype(getGlobal()->get("CurrentProcess")->get("prototype"));
    return object;
}
