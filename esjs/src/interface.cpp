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

using namespace es;

// #define VERBOSE

#ifndef VERBOSE
#define PRINTF(...)     (__VA_ARGS__)
#else
#define PRINTF(...)     report(__VA_ARGS__)
#endif

class ObjectValue;

class InterfaceStore
{
    static unsigned char* defaultInterfaceInfo[];

    Hashtable<Guid, Reflect::Interface> hashtable;

    void registerInterface(Reflect::Module& module);

public:
    InterfaceStore(int capacity = 128);

    Reflect::Interface& getInterface(const Guid& iid)
    {
        return hashtable.get(iid);
    }

    bool hasInterface(const Guid& iid)
    {
        return hashtable.contains(iid);
    }

    friend ObjectValue* constructSystemObject(void* system);
};

#include "interface.h"
#include "esjs.h"

extern ICurrentProcess* System();

//
// Reflection data of the default interface set
//

extern unsigned char IAlarmInfo[];
extern unsigned char ICacheInfo[];
extern unsigned char ICallbackInfo[];
extern unsigned char IClassFactoryInfo[];
extern unsigned char IClassStoreInfo[];
extern unsigned char IFileInfo[];
extern unsigned char IInterfaceInfo[];
extern unsigned char IInterfaceStoreInfo[];
extern unsigned char IMonitorInfo[];
extern unsigned char IPageableInfo[];
extern unsigned char IPageSetInfo[];
extern unsigned char IProcessInfo[];
extern unsigned char IRuntimeInfo[];
extern unsigned char ISelectableInfo[];
extern unsigned char IServiceInfo[];
extern unsigned char IStreamInfo[];
extern unsigned char IThreadInfo[];

extern unsigned char IAudioFormatInfo[];
extern unsigned char IBeepInfo[];
extern unsigned char ICursorInfo[];
extern unsigned char IDeviceInfo[];
extern unsigned char IDiskManagementInfo[];
extern unsigned char IDmacInfo[];
extern unsigned char IFileSystemInfo[];
extern unsigned char IPicInfo[];
extern unsigned char IRemovableMediaInfo[];
extern unsigned char IRtcInfo[];
extern unsigned char IPartitionInfo[];

extern unsigned char IBindingInfo[];
extern unsigned char IContextInfo[];

extern unsigned char IInternetAddressInfo[];
extern unsigned char IInternetConfigInfo[];
extern unsigned char IResolverInfo[];
extern unsigned char ISocketInfo[];

extern unsigned char IIteratorInfo[];
extern unsigned char ISetInfo[];

extern unsigned char ICanvasRenderingContext2DInfo[];

unsigned char* InterfaceStore::defaultInterfaceInfo[] =
{
    // Base classes first
    IInterfaceInfo,

    IAlarmInfo,
    ICacheInfo,
    ICallbackInfo,
    IClassFactoryInfo,
    IClassStoreInfo,
    IFileInfo,
    IInterfaceStoreInfo,
    IMonitorInfo,
    IPageableInfo,
    IPageSetInfo,
    IProcessInfo,
    IRuntimeInfo,
    ISelectableInfo,
    IServiceInfo,
    IStreamInfo,
    IThreadInfo,

    IAudioFormatInfo,
    IBeepInfo,
    ICursorInfo,
    IDeviceInfo,
    IDiskManagementInfo,
    IDmacInfo,
    IFileSystemInfo,
    IPicInfo,
    IRemovableMediaInfo,
    IRtcInfo,
    IPartitionInfo,

    IBindingInfo,
    IContextInfo,

    IInternetAddressInfo,
    IInternetConfigInfo,
    IResolverInfo,
    ISocketInfo,

    IIteratorInfo,
    ISetInfo,

    ICanvasRenderingContext2DInfo,
};

namespace
{
    InterfaceStore  interfaceStore;
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

void InterfaceStore::
registerInterface(Reflect::Module& module)
{
    for (int i = 0; i < module.getInterfaceCount(); ++i)
    {
        Reflect::Interface interface(module.getInterface(i));
        hashtable.add(interface.getIid(), interface);
    }

    for (int i = 0; i < module.getModuleCount(); ++i)
    {
        Reflect::Module m(module.getModule(i));
        registerInterface(m);
    }
}

InterfaceStore::
InterfaceStore(int capacity) :
    hashtable(capacity)
{
    for (int i = 0;
         i < sizeof defaultInterfaceInfo / sizeof defaultInterfaceInfo[0];
         ++i)
    {
        Reflect r(defaultInterfaceInfo[i]);
        Reflect::Module global(r.getGlobalModule());
        registerInterface(global);
    }
}

Reflect::Interface& getInterface(const Guid& iid)
{
    return interfaceStore.getInterface(iid);
}

//
// InterfacePointerValue
//

class InterfacePointerValue : public ObjectValue
{
    IInterface* object;

public:
    InterfacePointerValue(IInterface* object) :
        object(object)
    {
    }

    ~InterfacePointerValue()
    {
        if (object)
        {
            object->release();
        }
    }

    IInterface*& getObject()
    {
        return object;
    }

    void clearObject()
    {
        object = 0;
    }
};

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
    Param  argv[9];
    Param* argp = argv;
    int    ext = 0;
    Guid   riid = IInterface::iid();

    // Set this
    argp->ptr = self;
    argp->cls = Param::PTR;
    ++argp;

    // In the following implementation, we assume no out or inout attribute is
    // used for parameters.

    Reflect::Type returnType = method.getReturnType();
    switch (returnType.getType())
    {
    case Ent::TypeSequence:
        // int op(xxx* buf, int len, ...);
        argp->ptr = heap;
        argp->cls = Param::PTR;
        ++argp;
        ++ext;
        argp->s32 = (long) ((*list)[0])->toNumber();
        argp->cls = Param::S32;
        ++argp;
        break;
    case Ent::SpecString:
    case Ent::SpecWString:
        // int op(xxx* buf, int len, ...);
        argp->ptr = heap;
        argp->cls = Param::PTR;
        ++argp;
        argp->s32 = sizeof(heap);
        argp->cls = Param::S32;
        ++argp;
        break;
    case Ent::SpecUuid:
    case Ent::TypeStructure:
        // void op(struct* buf, ...);
        argp->ptr = heap;
        argp->cls = Param::PTR;
        ++argp;
        break;
    case Ent::TypeArray:
        // void op(xxx[x] buf, ...);
        argp->ptr = heap;
        argp->cls = Param::PTR;
        ++argp;
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
        case Ent::TypeSequence:
            // xxx* buf, int len, ...
            // XXX Assume sequence<octet> now...
            argp->ptr = value->toString().c_str();
            argp->cls = Param::PTR;
            ++argp;
            value = (*list)[++i];
            argp->s32 = (s32) value->toNumber();
            argp->cls = Param::S32;
            break;
        case Ent::SpecString:
            argp->ptr = value->toString().c_str();
            argp->cls = Param::PTR;
            break;
        case Ent::SpecWString:
            // XXX
            break;
        case Ent::SpecUuid:
            if (!parseGuid(value->toString().c_str(), &argp->guid))
            {
                throw getErrorInstance("TypeError");
            }
            argp->cls = Param::REF;
            riid = argp->guid;
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
            {
                InterfacePointerValue* unknown = dynamic_cast<InterfacePointerValue*>(value);
                if (unknown)
                {
                    argp->ptr = unknown->getObject();
                }
                else
                {
                    argp->ptr = 0;
                }
                argp->cls = Param::PTR;
            }
            break;
        case Ent::SpecBool:
            argp->s32 = (bool) value->toBoolean();
            argp->cls = Param::S32;
            break;
        case Ent::SpecAny:
            if (sizeof(s32) < sizeof(void*))
            {
                argp->s64 = (long long) value->toNumber();
                argp->cls = Param::S64;
            }
            else
            {
                argp->s32 = (long) value->toNumber();
                argp->cls = Param::S32;
            }
            break;
        case Ent::SpecS8:
        case Ent::SpecS16:
        case Ent::SpecS32:
        case Ent::SpecU8:
        case Ent::SpecU16:
        case Ent::SpecU32:
            argp->s32 = (long) value->toNumber();
            argp->cls = Param::S32;
            break;
        case Ent::SpecS64:
        case Ent::SpecU64:
            argp->s64 = (long long) value->toNumber();
            argp->cls = Param::S64;
            break;
        case Ent::SpecF32:
            argp->f32 = (float) value->toNumber();
            argp->cls = Param::F32;
            break;
        case Ent::SpecF64:
            argp->f64 = (double) value->toNumber();
            argp->cls = Param::F64;
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
    case Ent::SpecBool:
        value = BoolValue::getInstance(applyS32(argc, argv, (s32 (*)()) ((*self)[methodNumber])) ? true : false);
        break;
    case Ent::SpecChar:
    case Ent::SpecWChar:
    case Ent::SpecS8:
    case Ent::SpecS16:
    case Ent::SpecS32:
    case Ent::SpecU8:
    case Ent::SpecU16:
    case Ent::SpecU32:
        value = new NumberValue(applyS32(argc, argv, (s32 (*)()) ((*self)[methodNumber])));
        break;
    case Ent::SpecS64:
    case Ent::SpecU64:
        value = new NumberValue(applyS64(argc, argv, (s64 (*)()) ((*self)[methodNumber])));
        break;
    case Ent::SpecAny:
        if (sizeof(s32) < sizeof(void*))
        {
            value = new NumberValue(applyS64(argc, argv, (s64 (*)()) ((*self)[methodNumber])));
        }
        else
        {
            value = new NumberValue(applyS32(argc, argv, (s32 (*)()) ((*self)[methodNumber])));
        }
        break;
    case Ent::SpecF32:
        value = new NumberValue(applyF32(argc, argv, (f32 (*)()) ((*self)[methodNumber])));
        break;
    case Ent::SpecF64:
        value = new NumberValue(applyF64(argc, argv, (f64 (*)()) ((*self)[methodNumber])));
        break;
    case Ent::SpecString:
        {
            int result = applyS32(argc, argv, (s32 (*)()) ((*self)[methodNumber]));
            if (result < 0)
            {
                heap[0] = '\0';
            }
            value = new StringValue(heap);
        }
        break;
    case Ent::TypeSequence:
        {
            // XXX Assume sequence<octet> now...
            int count = applyS32(argc, argv, (s32 (*)()) ((*self)[methodNumber]));
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
        {
            IInterface* unknown = (IInterface*) applyPTR(argc, argv, (const void* (*)()) ((*self)[methodNumber]));
            if (unknown)
            {
                ObjectValue* instance = new InterfacePointerValue(unknown);
                instance->setPrototype(getGlobal()->get(getInterface(riid).getName())->get("prototype"));   // XXX Should use IID
                value = instance;
            }
            else
            {
                value = NullValue::getInstance();
            }
        }
        break;
    case Ent::SpecVoid:
        applyS32(argc, argv, (s32 (*)()) ((*self)[methodNumber]));
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
// InterfaceConstructor
//

class InterfaceConstructor : public Code
{
    FormalParameterList*    arguments;
    ObjectValue*            prototype;
    Guid                    iid;

public:
    InterfaceConstructor(ObjectValue* object, const Guid& iid) :
        arguments(new FormalParameterList),
        prototype(new ObjectValue),
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
    for (int i = 0;
         i < sizeof InterfaceStore::defaultInterfaceInfo / sizeof InterfaceStore::defaultInterfaceInfo[0];
         ++i)
    {
        Reflect r(InterfaceStore::defaultInterfaceInfo[i]);
        Reflect::Module global(r.getGlobalModule());
        constructSystemObject(global);
    }

    System()->addRef();
    ObjectValue* object = new InterfacePointerValue(System());
    object->setPrototype(getGlobal()->get("CurrentProcess")->get("prototype"));
    return object;
}
