/*
 * Copyright (c) 2006, 2007
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

#include <string.h>
#include <es/endian.h>
#include <es/formatter.h>
#include <es/base/IInterface.h>
#include <es/base/IProcess.h>
#include <es/hashtable.h>
#include <es/reflect.h>

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

void printType(Reflect::Type type)
{
    if (type.isConst())
    {
        report("const ");
    }

    report("%s", type.getName());

    if (type.getPointer() == 1)
    {
        report("*");
    }
    else if (type.getPointer() == 2)
    {
        report("**");
    }

    if (type.isReference())
    {
        report("&");
    }

    // We also need the byte count of each parameter.
    report(" {%d}", type.getSize());

    if (type.isPointer() || type.isReference())
    {
        report("-{%d}", type.getReferentSize());
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
        for (int j = 0; j < r.getInterfaceCount(); ++j)
        {
            if (r.getInterface(j).getType().isImported())
            {
                continue;
            }
            hashtable.add(r.getInterface(j).getIid(), r.getInterface(j));
        }
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
// InterfaceMethodCode
//

typedef long long (*InterfaceMethod)(void* self, ...);

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
        Reflect::Function method(interface.getMethod(number));

        // Add as many arguments as required.
        for (int i = 0; i < method.getParameterCount(); ++i)
        {
            Reflect::Identifier param(method.getParameter(i));
            if (param.isInput() || !param.isOutput())
            {
#ifdef VERBOSE
                report("    %s : ", param.getName());
                printType(param.getType());
                if (param.getType().isInteger())
                {
                    report(" : Integer");
                }
                if (param.getType().isFloat())
                {
                    report(" : Float");
                }
                if (param.getType().isBoolean())
                {
                    report(" : Boolean");
                }
                if (param.getType().isString())
                {
                    report(" : String");
                }
                if (param.getType().isUuid())
                {
                    report(" : UUID");
                }
                if (param.getType().isInterfacePointer())
                {
                    report(" : Interface Pointer");
                }
                report("\n");
#endif
                // Note the name "arguments" is reserved in a ECMAScript function.
                ASSERT(strcmp(param.getName(), "arguments") != 0);
                arguments->add(new Identifier(param.getName()));
            }
        }

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
        Reflect::Interface interface = getInterface(iid);
        Reflect::Function method(interface.getMethod(number));

        // Get this.
        InterfacePointerValue* object = dynamic_cast<InterfacePointerValue*>(getThis());
        if (!object)
        {
            throw getErrorInstance("TypeError");
        }

        // Copy parameters in
        Param  argv[9];
        Param* argp = argv;
        int    size;

        int    output = -1;

        // Set this
        InterfaceMethod** self = reinterpret_cast<InterfaceMethod**>(object->getObject());
        argp->ptr = self;
        argp->cls = Param::PTR;
        ++argp;

        unsigned baseMethodCount = interface.getTotalMethodCount() - interface.getMethodCount();

        PRINTF("evaluate %s.%s(%p) %d\n", interface.getName(), method.getName(), self, baseMethodCount);

        if (!self)
        {
            throw getErrorInstance("TypeError");
        }

        ListValue* list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
        for (int i = 0, j = 0; i < method.getParameterCount(); ++i, ++argp)
        {
            Reflect::Identifier param(method.getParameter(i));
            Reflect::Type type(param.getType());

            size = type.getSize();
            if (param.isInput() || !param.isOutput())
            {
                Value* value = (*list)[j++];
                if (type.isInteger())
                {
                    if (size == sizeof(s64))
                    {
                        argp->s64 = (long long) value->toNumber();
                        argp->cls = Param::S64;
                    }
                    else
                    {
                        argp->s32 = (long) value->toNumber();
                        argp->cls = Param::S32;
                    }
                }
                else if (type.isFloat())
                {
                    if (size == sizeof(f64))
                    {
                        argp->f64 = (double) value->toNumber();
                        argp->cls = Param::F64;
                    }
                    else
                    {
                        argp->f32 = (float) value->toNumber();
                        argp->cls = Param::F32;
                    }
                }
                else if (type.isBoolean())
                {
                    argp->s32 = (bool) value->toBoolean();
                    argp->cls = Param::S32;
                }
                else if (type.isUuid())
                {
                    if (!parseGuid(value->toString().c_str(), &argp->guid))
                    {
                        throw getErrorInstance("TypeError");
                    }
                    argp->cls = Param::REF;
                }
                else if (type.isInterfacePointer())
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
                else if (type.isString() || type.isPointer())   // XXX
                {
                    argp->ptr = value->toString().c_str();
                    argp->cls = Param::PTR;
                }
            }
            else if (param.isOutput())
            {
                output = i;
                if (param.isInterfacePointer())
                {
                    argp->ptr = 0;
                    argp->cls = Param::REF;
                }
                else if (type.isString() || type.isPointer() || type.isReference())   // XXX
                {
                    // Determine the size of the object pointed.
                    int count(0);
                    if (0 <= param.getSizeIs())
                    {
                        count = (int) ((*list)[param.getSizeIs() - 1]->toNumber()); // XXX - 1
                    }
                    else
                    {
                        // Determine the size of object pointed by this parameter.
                        count = param.getType().getReferentSize();
                    }
                    argp->ptr = malloc(count + 1);
                    argp->cls = Param::PTR;
                }
                else
                {
                    ASSERT(0);  // XXX
                }
            }
        }

        // Invoke method
        Reflect::Type returnType(method.getReturnType());
        size = returnType.getSize();
        Param result;
        Register<Value> value;

        if (returnType.isInteger())
        {
            if (size == sizeof(s64))
            {
                value = new NumberValue(applyS64(1 + method.getParameterCount(), argv, (s64 (*)()) ((*self)[baseMethodCount + number])));
            }
            else
            {
                value = new NumberValue(applyS32(1 + method.getParameterCount(), argv, (s32 (*)()) ((*self)[baseMethodCount + number])));
            }
        }
        else if (returnType.isFloat())
        {
            if (size == sizeof(f64))
            {
                value = new NumberValue(applyF64(1 + method.getParameterCount(), argv, (f64 (*)()) ((*self)[baseMethodCount + number])));
            }
            else
            {
                value = new NumberValue(applyF32(1 + method.getParameterCount(), argv, (f32 (*)()) ((*self)[baseMethodCount + number])));
            }
        }
        else if (returnType.isBoolean())
        {
            bool r = applyS32(1 + method.getParameterCount(), argv, (s32 (*)()) ((*self)[baseMethodCount + number]));
            value = BoolValue::getInstance(r ? true : false);
        }
        else if (returnType.isString())
        {
            value = new StringValue((char*) applyPTR(1 + method.getParameterCount(), argv, (const void* (*)()) ((*self)[baseMethodCount + number])));
        }
        else if (returnType.isInterfacePointer())
        {
            IInterface* unknown = (IInterface*) applyPTR(1 + method.getParameterCount(), argv, (const void* (*)()) ((*self)[baseMethodCount + number]));
            if (unknown)
            {
                ObjectValue* instance = new InterfacePointerValue(unknown);
                instance->setPrototype(getGlobal()->get(returnType.getName())->get("prototype"));   // XXX Should use IID
                value = instance;
            }
            else
            {
                value = NullValue::getInstance();
            }
        }
        else
        {
            applyS32(1 + method.getParameterCount(), argv, (s32 (*)()) ((*self)[baseMethodCount + number]));
            value = NullValue::getInstance();
        }

        // Process output
        if (0 <= output)
        {
            Reflect::Identifier param(method.getParameter(output));
            Reflect::Type type(param.getType());
            if (param.isInterfacePointer())
            {
                IInterface* unknown = (IInterface*) argv[1 + output].ptr;
                if (unknown)
                {
                    ObjectValue* instance = new InterfacePointerValue(unknown);
                    if (0 <= param.getIidIs())
                    {
                        instance->setPrototype(getGlobal()->get(getInterface(argv[1 + param.getIidIs()].guid).getName())->get("prototype"));   // XXX Should use IID
                    }
                    else
                    {
                        instance->setPrototype(getGlobal()->get(type.getName())->get("prototype"));   // XXX Should use IID
                    }
                    value = instance;
                }
                else
                {
                    value = NullValue::getInstance();
                }
            }
            else if (type.isString() || type.isPointer() || type.isReference())   // XXX
            {
                int count = (int) value->toNumber();
                if (0 <= count)
                {
                    argp = &argv[1 + output];   // 1 for this
                    ((char*) (argp->ptr))[count] = '\0';
                    value = new StringValue((char*) argp->ptr);
                }
                free((void*) argp->ptr);
            }
            else
            {
                ASSERT(0);  // XXX
            }
        }

        if (iid == IID_IInterface && number == 2)   // IInterface::release()
        {
            object->clearObject();
        }

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
            Reflect::Function method(interface.getMethod(i));
            if (prototype->hasProperty(method.getName()))
            {
                // XXX Currently overloaded functions are just ignored.
                continue;
            }
            PRINTF("function: %s\n", method.getName());
            ObjectValue* function = new ObjectValue;
            function->setCode(new InterfaceMethodCode(function, iid, i));
            prototype->put(method.getName(), function);
        }

        if (interface.getSuperIid() == GUID_NULL)
        {
            prototype->setPrototype(getGlobal()->get("Interface")->getPrototype()->getPrototype());
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
        if (!object || !object->queryInterface(iid, (void**) &object))
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

ObjectValue* constructSystemObject(void* system)
{
    for (int i = 0;
         i < sizeof InterfaceStore::defaultInterfaceInfo / sizeof InterfaceStore::defaultInterfaceInfo[0];
         ++i)
    {
        Reflect r(InterfaceStore::defaultInterfaceInfo[i]);
        for (int j = 0; j < r.getInterfaceCount(); ++j)
        {
            if (r.getInterface(j).getType().isImported())
            {
                continue;
            }

            // Construct Default Interface Object
            PRINTF("%s\n", r.getInterface(j).getName());
            ObjectValue* object = new ObjectValue;
            object->setCode(new InterfaceConstructor(object, r.getInterface(j).getIid()));
            object->setPrototype(getGlobal()->get("Interface")->getPrototype());
            getGlobal()->put(r.getInterface(j).getName(), object);
        }
    }

    System()->addRef();
    ObjectValue* object = new InterfacePointerValue(System());
    object->setPrototype(getGlobal()->get("ICurrentProcess")->get("prototype"));
    return object;
}
