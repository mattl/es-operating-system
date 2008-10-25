/*
 * Copyright 2008 Google Inc.
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
#include <es/endian.h>
#include <es/formatter.h>
#include <es/base/IInterface.h>
#include <es/base/IProcess.h>
#include <es/hashtable.h>
#include <es/reflect.h>
#include <es/variant.h>

using namespace es;

// #define VERBOSE

#ifndef VERBOSE
#define PRINTF(...)     (__VA_ARGS__)
#else
#define PRINTF(...)     report(__VA_ARGS__)
#endif

class ObjectValue;

#include "interface.h"

extern ICurrentProcess* System();

// TODO use proper name prefix or namespace
extern Reflect::Interface& getInterface(const Guid& iid);
extern unsigned char* defaultInterfaceInfo[];
extern size_t defaultInterfaceCount;

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

void printGuid(const Guid& guid)
{
    report("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
           guid.Data1, guid.Data2, guid.Data3,
           guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
           guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

//
// invoke
//

typedef long long (*InterfaceMethod)(void* self, ...);

static char heap[64*1024];

static Value* invoke(Guid& iid, int number, InterfacePointerValue* object, ListValue* list)
{
    InterfaceMethod** self = reinterpret_cast<InterfaceMethod**>(object->getObject());
    if (!self)
    {
        throw getErrorInstance("TypeError");
    }

    Reflect::Interface interface = getInterface(iid);
    Reflect::Method method(interface.getMethod(number));
    PRINTF("invoke %s.%s(%p)\n", interface.getName(), method.getName(), self);

    // Set up parameters
    Variant argv[9];
    Variant* argp = argv;
    Guid iidv[9];
    Guid* iidp = iidv;
    int ext = 0;    // extra parameter count
    Guid riid = IInterface::iid();

    // Set this
    *argp++ = Variant(reinterpret_cast<intptr_t>(self));

    // In the following implementation, we assume no out nor inout attribute is
    // used for parameters.

    Reflect::Type returnType = method.getReturnType();
    switch (returnType.getType())
    {
    case Ent::SpecVariant:
        // Variant op(void* buf, int len, ...);
        // FALL THROUGH
    case Ent::SpecString:
        // int op(xxx* buf, int len, ...);
        *argp++ = Variant(reinterpret_cast<intptr_t>(heap));
        *argp++ = Variant(sizeof(heap));
        break;
    case Ent::TypeSequence:
        // int op(xxx* buf, int len, ...);
        *argp++ = Variant(reinterpret_cast<intptr_t>(heap));
        ++ext;
        *argp++ = Variant(static_cast<int32_t>(((*list)[0])->toNumber()));
        break;
    case Ent::SpecUuid:
    case Ent::TypeStructure:
        // void op(struct* buf, ...);
        *argp++ = Variant(reinterpret_cast<intptr_t>(heap));
        break;
    case Ent::TypeArray:
        // void op(xxx[x] buf, ...);
        *argp++ = Variant(reinterpret_cast<intptr_t>(heap));
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
            // Variant variant, ...
            switch (value->getType()) {
            case Value::BoolType:
                *argp = Variant(static_cast<bool>(value->toBoolean()));
                break;
            case Value::StringType:
                *argp = Variant(value->toString().c_str());
                break;
            case Value::NumberType:
                *argp = Variant(static_cast<double>(value->toNumber()));
                break;
            case Value::ObjectType:
                if (InterfacePointerValue* unknown = dynamic_cast<InterfacePointerValue*>(value))
                {
                    *argp = Variant(unknown->getObject());
                }
                else
                {
                    // XXX expose ECMAScript object
                    *argp = Variant(static_cast<IInterface*>(0));
                }
                break;
            default:
                *argp = Variant();
                break;
            }
            argp->makeVariant();
            break;
        case Ent::TypeSequence:
            // xxx* buf, int len, ...
            // XXX Assume sequence<octet> now...
            *argp++ = Variant(reinterpret_cast<intptr_t>(value->toString().c_str()));
            value = (*list)[++i];
            *argp = Variant(static_cast<int32_t>(value->toNumber()));
            break;
        case Ent::SpecString:
            *argp = Variant(value->toString().c_str());
            break;
        case Ent::SpecUuid:
            if (!parseGuid(value->toString().c_str(), iidp))
            {
                throw getErrorInstance("TypeError");
            }
            *argp = Variant(iidp);
            riid = *iidp;
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
                *argp = Variant(unknown->getObject());
            }
            else
            {
                *argp = Variant(static_cast<IInterface*>(0));
            }
            break;
        case Ent::SpecBool:
            *argp = Variant(static_cast<bool>(value->toBoolean()));
            break;
        case Ent::SpecAny:
            *argp = Variant(static_cast<intptr_t>(value->toNumber()));
            break;
        case Ent::SpecS16:
            *argp = Variant(static_cast<int16_t>(value->toNumber()));
            break;
        case Ent::SpecS32:
            *argp = Variant(static_cast<int32_t>(value->toNumber()));
            break;
        case Ent::SpecS8:
        case Ent::SpecU8:
            *argp = Variant(static_cast<uint8_t>(value->toNumber()));
            break;
        case Ent::SpecU16:
            *argp = Variant(static_cast<uint16_t>(value->toNumber()));
            break;
        case Ent::SpecU32:
            *argp = Variant(static_cast<uint32_t>(value->toNumber()));
            break;
        case Ent::SpecS64:
            *argp = Variant(static_cast<int64_t>(value->toNumber()));
            break;
        case Ent::SpecU64:
            *argp = Variant(static_cast<uint64_t>(value->toNumber()));
            break;
        case Ent::SpecF32:
            *argp = Variant(static_cast<float>(value->toNumber()));
            break;
        case Ent::SpecF64:
            *argp = Variant(static_cast<double>(value->toNumber()));
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
            Variant result = apply(argc, argv, (Variant (*)()) ((*self)[methodNumber]));
            switch (result.getType())
            {
            case Variant::TypeVoid:
                value = NullValue::getInstance();
                break;
            case Variant::TypeBool:
                value = BoolValue::getInstance(static_cast<bool>(result));
                break;
            case Variant::TypeOctet:
                value = new NumberValue(static_cast<uint8_t>(result));
                break;
            case Variant::TypeShort:
                value = new NumberValue(static_cast<int16_t>(result));
                break;
            case Variant::TypeUnsignedShort:
                value = new NumberValue(static_cast<uint16_t>(result));
                break;
            case Variant::TypeLong:
                value = new NumberValue(static_cast<int32_t>(result));
                break;
            case Variant::TypeUnsignedLong:
                value = new NumberValue(static_cast<uint32_t>(result));
                break;
            case Variant::TypeLongLong:
                value = new NumberValue(static_cast<int64_t>(result));
                break;
            case Variant::TypeUnsignedLongLong:
                value = new NumberValue(static_cast<uint64_t>(result));
                break;
            case Variant::TypeFloat:
                value = new NumberValue(static_cast<float>(result));
                break;
            case Variant::TypeDouble:
                value = new NumberValue(static_cast<double>(result));
                break;
            case Variant::TypeString:
                value = new StringValue(heap);
                break;
            case Variant::TypeObject:
                if (IInterface* unknown = static_cast<IInterface*>(result))
                {
                    ObjectValue* instance = new InterfacePointerValue(unknown);
                    instance->setPrototype(getGlobal()->get(getInterface(riid).getName())->get("prototype"));   // XXX Should use IID
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
        heap[0] = '\0';
        apply(argc, argv, (int32_t (*)()) ((*self)[methodNumber]));
        value = new StringValue(heap);
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
        riid = returnType.getInterface().getIid();
        // FALL THROUGH
    case Ent::SpecObject:
        if (IInterface* unknown = apply(argc, argv, (IInterface* (*)()) ((*self)[methodNumber])))
        {
            ObjectValue* instance = new InterfacePointerValue(unknown);
            instance->setPrototype(getGlobal()->get(getInterface(riid).getName())->get("prototype"));   // XXX Should use IID
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

    if (iid == IInterface::iid() && number == 2)   // IInterface::release()
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
    bool    readOnly;
    Guid    iid;
    int     getter;     // Method number
    int     setter;     // Method number

public:
    AttributeValue(const Guid& iid) :
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

    Guid                    iid;
    int                     number;     // Method number

public:
    InterfaceMethodCode(ObjectValue* object, const Guid& iid, int number) :
        arguments(new FormalParameterList),
        prototype(new ObjectValue),
        iid(iid),
        number(number)
    {
        Reflect::Interface interface = getInterface(iid);
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
    Guid iid;
    int number; // Method number

public:
    AttributeSetterValue(const Guid& iid, int number) :
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
    Guid iid;
    int number; // Method number

public:
    AttributeGetterValue(const Guid& iid, int number) :
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
    FormalParameterList*      arguments;
    InterfacePrototypeValue*  prototype;
    Guid                      iid;

public:
    InterfaceConstructor(ObjectValue* object, const Guid& iid) :
        arguments(new FormalParameterList),
        prototype(new InterfacePrototypeValue),
        iid(iid)
    {
        arguments->add(new Identifier("object"));

        object->setParameterList(arguments);
        object->setScope(getGlobal());

        Reflect::Interface interface = getInterface(iid);
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

        if (interface.getSuperIid() == GUID_NULL)
        {
            prototype->setPrototype(getGlobal()->get("InterfaceStore")->getPrototype()->getPrototype());
        }
        else
        {
            Reflect::Interface super = getInterface(interface.getSuperIid());
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
        InterfacePointerValue* self = dynamic_cast<InterfacePointerValue*>(getScopeChain()->get("object"));
        if (!self)
        {
            throw getErrorInstance("TypeError");
        }

        IInterface* object;
        object = self->getObject();
        if (!object || !(object = reinterpret_cast<IInterface*>(object->queryInterface(iid))))
        {
            // We should throw an error in case called by a new expression.
            throw getErrorInstance("TypeError");
        }

        ObjectValue* value = new InterfacePointerValue(object);
        value->setPrototype(prototype);
        return CompletionType(CompletionType::Return, value, "");
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

        Guid iid;
        if (!parseGuid(value->toString().c_str(), &iid))
        {
            throw getErrorInstance("TypeError");
        }

        Reflect::Interface interface;
        try
        {
            interface = getInterface(iid);
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
        PRINTF("%s\n", interface.getName());
        ObjectValue* object = new ObjectValue;
        object->setCode(new InterfaceConstructor(object, interface.getIid()));
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
    for (int i = 0; i < defaultInterfaceCount; ++i)
    {
        Reflect r(defaultInterfaceInfo[i]);
        Reflect::Module global(r.getGlobalModule());
        constructSystemObject(global);
    }

    System()->addRef();
    ObjectValue* object = new InterfacePointerValue(System());
    object->setPrototype(getGlobal()->get("CurrentProcess")->get("prototype"));
    return object;
}
