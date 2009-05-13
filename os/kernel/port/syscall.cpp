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

#include <errno.h>
#include <stddef.h>
#include <es.h>
#include <es/any.h>
#include <es/broker.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/reflect.h>
#include <es/base/ISelectable.h>
#include <es/net/IInternetAddress.h>
#include <es/net/IInternetConfig.h>
#include <es/net/ISocket.h>
#include <es/net/IResolver.h>
#include "core.h"
#include "interfaceStore.h"
#include "process.h"

// #define VERBOSE

extern es::Stream* esReportStream();

typedef long long (*Method)(void* self, ...);

bool SyscallProxy::set(void* object, const char* iid, bool used)
{
    if (ref.addRef() != 1)
    {
        ref.release();
        return false;
    }
    this->object = object;
    this->iid = es::getUniqueIdentifier(iid);
    use.exchange(used ? 1 : 0);
    return true;
}

unsigned int SyscallProxy::addRef()
{
    return ref.addRef();
}

unsigned int SyscallProxy::release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        Object* object(static_cast<Object*>(getObject()));
        if (object)
        {
#ifdef VERBOSE
            Reflect::Interface interface = getInterface(iid);
            esReport("SyscallProxy::%s %s %p\n", __func__, interface.getName(), object);
#endif
            this->object = 0;
            object->release();
        }
    }
    return count;
}

long SyscallProxy::addUser()
{
    long count = use.increment();
    if (count == 1)
    {
        addRef();
    }
    return count;
}

long SyscallProxy::releaseUser()
{
    long count = use.decrement();
    if (count == 0)
    {
        release();
    }
    return count;
}

long long Process::
systemCall(void** self, unsigned methodNumber, va_list paramv, void** base)
{
    bool log = this->log;
    int* paramp = reinterpret_cast<int*>(paramv);

    if (base)
    {
        // Note base is zero if a system call is called from the "C" runtime.
        if (reinterpret_cast<int>(base) & (sizeof(void*) - 1))
        {
            throw SystemException<EBADF>();
        }
        ipt = base;
    }

    //
    // Determine the type of interface and which method is being invoked.
    //
    if (reinterpret_cast<long>(self) & (sizeof(void*) - 1))
    {
        throw SystemException<EBADF>();
    }
    unsigned interfaceNumber = self - base;
    Any* variant = 0;
    if (INTERFACE_POINTER_MAX <= interfaceNumber)
    {
        // self could be a pointer to a Any value when the method returns a Any.
        variant = reinterpret_cast<Any*>(self);
        if (!isValid(variant, sizeof(Any)))
        {
          throw SystemException<EBADF>();
        }
        if (!isValid(paramp, sizeof(void*)))
        {
          throw SystemException<EBADF>();
        }
        self = *reinterpret_cast<void***>(paramp);
        ++paramp;
        interfaceNumber = self - base;
        if (INTERFACE_POINTER_MAX <= interfaceNumber) {
          throw SystemException<EBADF>();
        }
    }
    Handle<SyscallProxy> proxy(&syscallTable[interfaceNumber], true);
    if (!proxy->isValid())
    {
        throw SystemException<EBADF>();
    }

    Reflect::Interface interface = es::getInterface(proxy->iid);   // XXX Should cache the result.

    // Suppress unwanted trace outputs.
    if (proxy->getObject() == esReportStream() ||
        interfaceNumber == 0 && methodNumber == 15) // es::Process::getNow()
    {
        log = false;
    }

    if (log)
    {
        esReport("system call[%d:%p]: %s", interfaceNumber, this, interface.getName());
    }

    // If this interface inherits another interface,
    // methodNumber is checked accordingly.
    if (interface.getInheritedMethodCount() + interface.getMethodCount() <= methodNumber)
    {
        if (log)
        {
            esReport("\n");
        }
        throw SystemException<ENOSYS>();
    }
    unsigned baseMethodCount;
    Reflect::Interface super(interface);
    for (;;)
    {
        baseMethodCount = super.getInheritedMethodCount();
        if (baseMethodCount <= methodNumber)
        {
            break;
        }
        super = es::getInterface(super.getFullyQualifiedSuperName());
    }

    Reflect::Method method(super.getMethod(methodNumber - baseMethodCount));
    Reflect::Type returnType = method.getReturnType();
    if (variant && returnType.getType() != Ent::SpecVariant ||
        !variant && returnType.getType() == Ent::SpecVariant)
    {
        throw SystemException<EBADF>();
    }

    if (log)
    {
        esReport("::%s(", method.getName());
    }

    // Process addRef() and release() locally
    bool stringIsInterfaceName = false;
    if (super.getFullyQualifiedSuperName() == 0)
    {
        unsigned long count;
        switch (methodNumber - baseMethodCount)
        {
        case 0:  // queryInterface
            stringIsInterfaceName = true;
            break;
        case 1:  // addRef
            count = proxy->addUser();
            if (log)
            {
                esReport("%p) : %d;\n", proxy.get(), count);
            }
            return count;
            break;
        case 2:  // release
            count = proxy->releaseUser();
            if (log)
            {
                esReport("%p) : %d;\n", proxy.get(), count);
            }
            return count;
            break;
        }
    }

    //
    // Set up parameters
    //
    Any argv[9];
    Any* argp = argv;

    void* ptr;
    int count;

    Handle<SyscallProxy> inputProxies[8];
    Handle<UpcallProxy>  upcallProxies[8];

    const char* iid = Object::iid();

    // Set this
    Method** object = reinterpret_cast<Method**>(proxy->getObject());
    *argp++ = Any(reinterpret_cast<intptr_t>(object));

    switch (returnType.getType())
    {
    case Ent::SpecVariant:
        //  Any op(void* buf, int len);
    case Ent::SpecString:
        // const char* op(char* buf, int len, ...);
        if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
        {
            throw SystemException<EFAULT>();
        }
        ptr = *reinterpret_cast<void**>(paramp);
        *argp++ = Any(reinterpret_cast<intptr_t>(ptr));
        ++paramp;
        if (!isValid(paramp, sizeof(int32_t)))
        {
            throw SystemException<EFAULT>();
        }
        count = *paramp;
        *argp++ = Any(static_cast<int32_t>(count));
        ++paramp;
        break;
    case Ent::TypeSequence:
        // int op(xxx* buf, int len, ...);
        if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
        {
            throw SystemException<EFAULT>();
        }
        ptr = *reinterpret_cast<void**>(paramp);
        *argp++ = Any(reinterpret_cast<intptr_t>(ptr));
        ++paramp;
        if (!isValid(paramp, sizeof(int32_t)))
        {
            throw SystemException<EFAULT>();
        }
        *argp++ = Any(static_cast<int32_t>(*paramp));
        count = *paramp * returnType.getSize();
        ++paramp;
        break;
    case Ent::TypeStructure:
        // void op(struct* buf, ...);
        if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
        {
            throw SystemException<EFAULT>();
        }
        ptr = *reinterpret_cast<void**>(paramp);
        *argp++ = Any(reinterpret_cast<intptr_t>(ptr));
        count = returnType.getSize();
        ++paramp;
        break;
    case Ent::TypeArray:
        // void op(xxx[x] buf, ...);
        if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
        {
            throw SystemException<EFAULT>();
        }
        ptr = *reinterpret_cast<void**>(paramp);
        *argp++ = Any(reinterpret_cast<intptr_t>(ptr));
        count = returnType.getSize();
        ++paramp;
        break;
    default:
        ptr = 0;
        count = 0;
        break;
    }
    if (ptr && !isValid(ptr, count))
    {
        throw SystemException<EFAULT>();
    }

    for (int i = 0; i < method.getParameterCount(); ++i, ++argp)
    {
        Reflect::Parameter param(method.getParameter(i));
        Reflect::Type type(param.getType());
        assert(param.isInput());

        const void* ptr = 0;
        int count = 0;

        u32 entType = type.getType();
        u32 stackBytes;
        switch (entType)
        {
        case Ent::SpecS64:
        case Ent::SpecU64:
        case Ent::SpecF64:
        case Ent::TypeSequence:
            stackBytes = 8;
            break;
        case Ent::SpecVariant:
            stackBytes = sizeof(AnyBase);
            break;
        default:
            stackBytes = 4;
            break;
        }
        if (!isValid(paramp, stackBytes))
        {
            throw SystemException<EFAULT>();
        }

        switch (entType)
        {
        case Ent::SpecVariant:
            *argp = Any(*reinterpret_cast<AnyBase*>(paramp));
            paramp += sizeof(AnyBase) / sizeof(int);
            switch (argp->getType())
            {
            case Any::TypeString:
                ptr = static_cast<const char*>(*argp);
                count = sizeof(char);       // XXX check string length?
                break;
            case Any::TypeObject:
                if (void** ip = *reinterpret_cast<void***>(static_cast<Object*>(*argp)))
                {
                    if (base <= ip && ip < base + INTERFACE_POINTER_MAX)
                    {
                        unsigned interfaceNumber(ip - base);
                        Handle<SyscallProxy> proxy(&syscallTable[interfaceNumber], true);
                        if (!proxy->isValid())
                        {
                            throw SystemException<EINVAL>();
                        }
                        inputProxies[i] = proxy;
                        *argp = Any(static_cast<Object*>(inputProxies[i]->getObject()));
                    }
                    else    // XXX Check range
                    {
                        // Allocate an entry in the upcall table and set the
                        // interface pointer to the broker for the upcall table.
                        int n = set(this, (Object*) ip, iid, false);
                        if (n < 0)
                        {
                            throw SystemException<ENFILE>();
                        }
                        // Note the reference count of the created upcall proxy must
                        // be decremented by one at the end of this system call.
                        upcallProxies[i] = &upcallTable[n];
                        *argp = Any(reinterpret_cast<Object*>(&(broker.getInterfaceTable())[n]));
                    }
                }
                else
                {
                    *argp = Any(static_cast<Object*>(0));
                }
                argp->makeVariant();
                break;
            }
            break;
        case Ent::SpecBool:
            *argp = Any(static_cast<bool>(*paramp++));
            break;
        case Ent::SpecAny:
            *argp = Any(static_cast<uint32_t>(*paramp++)); // x86 only
            break;
        case Ent::SpecS16:
            *argp = Any(static_cast<int16_t>(*paramp++));
            break;
        case Ent::SpecS32:
            *argp = Any(static_cast<int32_t>(*paramp++));
            break;
        case Ent::SpecS8:
        case Ent::SpecU8:
            *argp = Any(static_cast<uint8_t>(*paramp++));
            break;
        case Ent::SpecU16:
            *argp = Any(static_cast<uint16_t>(*paramp++));
            break;
        case Ent::SpecU32:
            *argp = Any(static_cast<uint32_t>(*paramp++));
            break;
        case Ent::SpecS64:
            *argp = Any(*reinterpret_cast<int64_t*>(paramp));
            paramp += 2;
            break;
        case Ent::SpecU64:
            *argp = Any(*reinterpret_cast<uint64_t*>(paramp));
            paramp += 2;
            break;
        case Ent::SpecF32:
            *argp = Any(*reinterpret_cast<float*>(paramp++));
            break;
        case Ent::SpecF64:
            *argp = Any(*reinterpret_cast<double*>(paramp));
            paramp += 2;
            break;
        case Ent::SpecString:
            ptr = *reinterpret_cast<void**>(paramp);
            if (stringIsInterfaceName)
            {
                iid = static_cast<const char*>(ptr);
            }
            *argp = Any(static_cast<const char*>(ptr));
            ++paramp;
            count = sizeof(char);       // XXX check string length?
            break;
        case Ent::TypeSequence:
            // xxx* buf, int len, ...
            ptr = *reinterpret_cast<void**>(paramp);
            *argp++ = Any(reinterpret_cast<intptr_t>(ptr));
            ++paramp;
            *argp = Any(static_cast<int32_t>(*paramp));
            count = *paramp * type.getSize();
            ++paramp;
            break;
        case Ent::TypeStructure:    // struct* buf, ...
        case Ent::TypeArray:        // xxx[x] buf, ...
            ptr = *reinterpret_cast<void**>(paramp);
            *argp = Any(reinterpret_cast<intptr_t>(ptr));
            count = type.getSize();
            ++paramp;
            break;
        case Ent::TypeInterface:
            iid = type.getInterface().getFullyQualifiedName();
            // FALL THROUGH
        case Ent::SpecObject:
            if (void** ip = *reinterpret_cast<void***>(paramp++))
            {
                if (base <= ip && ip < base + INTERFACE_POINTER_MAX)
                {
                    unsigned interfaceNumber(ip - base);
                    Handle<SyscallProxy> proxy(&syscallTable[interfaceNumber], true);
                    if (!proxy->isValid())
                    {
                        throw SystemException<EINVAL>();
                    }
                    inputProxies[i] = proxy;
                    *argp = Any(static_cast<Object*>(inputProxies[i]->getObject()));
                }
                else    // XXX Check range
                {
                    // Allocate an entry in the upcall table and set the
                    // interface pointer to the broker for the upcall table.
                    int n = set(this, (Object*) ip, iid, false);
                    if (n < 0)
                    {
                        throw SystemException<ENFILE>();
                    }
                    // Note the reference count of the created upcall proxy must
                    // be decremented by one at the end of this system call.
                    upcallProxies[i] = &upcallTable[n];
                    *argp = Any(reinterpret_cast<Object*>(&(broker.getInterfaceTable())[n]));
                }
            }
            else
            {
                *argp = Any(static_cast<Object*>(0));
            }
            break;
        default:
            break;
        }

        // Check range
        if (ptr && !isValid(ptr, count))
        {
            throw SystemException<EFAULT>();
        }

        if (log && i + 1 < method.getParameterCount())
        {
            esReport(", ");
        }
    }

    if (log)
    {
        esReport(");\n");
    }

    // Invoke method
    int argc = argp - argv;
    Any result;
    void* ip = 0;
    switch (returnType.getType())
    {
    case Ent::SpecVariant:
        *variant = apply(argc, argv, (Any (*)()) ((*object)[methodNumber]));
        result = Any(reinterpret_cast<intptr_t>(variant));
        if (variant->getType() == Any::TypeObject) {
            ip = static_cast<Object*>(*variant);
        }
        break;
    case Ent::SpecBool:
        result = apply(argc, argv, (bool (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecChar:
    case Ent::SpecS8:
    case Ent::SpecU8:
        result = apply(argc, argv, (uint8_t (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecWChar:
    case Ent::SpecS16:
        result = apply(argc, argv, (int16_t (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecU16:
        result = apply(argc, argv, (uint16_t (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecS32:
        result = apply(argc, argv, (int32_t (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecAny:  // XXX x86 specific
    case Ent::SpecU32:
        result = apply(argc, argv, (uint32_t (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecS64:
        result = apply(argc, argv, (int64_t (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecU64:
        result = apply(argc, argv, (uint64_t (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecF32:
        result = apply(argc, argv, (float (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecF64:
        result = apply(argc, argv, (double (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecString:
        result = apply(argc, argv, (const char* (*)()) ((*object)[methodNumber]));
        // TODO: If the returned string resides in the kernel space, it must be copied out.
        break;
    case Ent::TypeSequence:
        result = apply(argc, argv, (int32_t (*)()) ((*object)[methodNumber]));
        break;
    case Ent::TypeArray:
    case Ent::SpecVoid:
        apply(argc, argv, (int32_t (*)()) ((*object)[methodNumber]));
        break;
    case Ent::TypeInterface:
        if (!stringIsInterfaceName)
        {
            iid = returnType.getInterface().getFullyQualifiedName();
        }
        // FALL THROUGH
    case Ent::SpecObject:
        result = apply(argc, argv, (Object* (*)()) ((*object)[methodNumber]));
        ip = static_cast<Object*>(result);
        break;
    }

    // Process the returned interface pointer
    if (ip)
    {
        int n = set(syscallTable, ip, iid, true);
        if (0 <= n)
        {
            result = Any(reinterpret_cast<Object*>(&base[n]));
        }
        else
        {
            Object* object(static_cast<Object*>(ip));
            object->release();
            result = Any(static_cast<Object*>(0));
            throw SystemException<EMFILE>();
        }
    }

    // Process addRef() and release() locally
    if (interface.getFullyQualifiedName() == es::Monitor::iid())   // TODO(shiki): strcmp?
    {
        unsigned long count;
        switch (methodNumber)
        {
        case 3: // lock
            count = proxy->addUser();
            break;
        case 4: // tryLock
            if (static_cast<bool>(result))
            {
                count = proxy->addUser();
            }
            break;
        case 5: // unlock
            count = proxy->releaseUser();
            break;
        }
    }

    return evaluate(result);
}
