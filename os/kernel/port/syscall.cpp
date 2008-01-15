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

#include <errno.h>
#include <stddef.h>
#include <es.h>
#include <es/apply.h>
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

extern IStream* esReportStream();

typedef long long (*Method)(void* self, ...);

bool SyscallProxy::set(void* object, const Guid& iid, bool used)
{
    if (ref.addRef() != 1)
    {
        ref.release();
        return false;
    }
    this->object = object;
    this->iid = iid;
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
        IInterface* object(static_cast<IInterface*>(getObject()));
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
    bool log(this->log);

    if (base)
    {
        // Note base is zero if a system call is called from the "C" runtime.
        ipt = base;
        if (!isValid(base, sizeof(void*) * INTERFACE_POINTER_MAX))
        {
            throw SystemException<EFAULT>();
        }
    }

    //
    // Determine the type of interface and which method is being invoked.
    //
    if ((reinterpret_cast<long>(self) ^ reinterpret_cast<long>(base)) &
        (sizeof(void*) - 1))
    {
        throw SystemException<EBADF>();
    }
    unsigned interfaceNumber(self - base);

    if (INTERFACE_POINTER_MAX <= interfaceNumber)
    {
        throw SystemException<EBADF>();
    }
    Handle<SyscallProxy> proxy(&syscallTable[interfaceNumber], true);
    if (!proxy->isValid())
    {
        throw SystemException<EBADF>();
    }

    Reflect::Interface interface = getInterface(proxy->iid);   // XXX Should cache the result.

    // Suppress unwanted trace outputs.
    if (proxy->getObject() == esReportStream() ||
        interfaceNumber == 0 && methodNumber == 15) // IProcess::getNow()
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
        super = getInterface(super.getSuperIid());
    }
    Reflect::Method method(super.getMethod(methodNumber - baseMethodCount));
    if (log)
    {
        esReport("::%s(", method.getName());
    }

    // Process addRef() and release() locally
    if (super.getIid() == IInterface::iid())
    {
        unsigned long count;
        switch (methodNumber - baseMethodCount)
        {
        case 1: // addRef
            count = proxy->addUser();
            if (log)
            {
                esReport("%p) : %d;\n", proxy.get(), count);
            }
            return count;
            break;
        case 2: // release
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
    int* paramp = reinterpret_cast<int*>(paramv);
    Param argv[9];
    Param* argp = argv;

    void* ptr;
    int count;

    Handle<SyscallProxy> inputProxies[8];
    Handle<UpcallProxy>  upcallProxies[8];

    Guid iid = IInterface::iid();

    // Set this
    Method** object = reinterpret_cast<Method**>(proxy->getObject());
    argp->ptr = object;
    argp->cls = Param::PTR;
    ++argp;

    Reflect::Type returnType = method.getReturnType();
    switch (returnType.getType())
    {
    case Ent::SpecString:
        // int op(char* buf, int len, ...);
        if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
        {
            throw SystemException<EFAULT>();
        }
        argp->ptr = ptr = *reinterpret_cast<void**>(paramp);
        argp->cls = Param::PTR;
        ++argp;
        ++paramp;
        if (!isValid(paramp, sizeof(int)))
        {
            throw SystemException<EFAULT>();
        }
        argp->s32 = *paramp;
        argp->cls = Param::S32;
        count = argp->s32;
        ++argp;
        ++paramp;
        break;
    case Ent::SpecWString:
        // int op(wchar_t* buf, int len, ...);
        if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
        {
            throw SystemException<EFAULT>();
        }
        argp->ptr = ptr = *reinterpret_cast<void**>(paramp);
        argp->cls = Param::PTR;
        ++argp;
        ++paramp;
        if (!isValid(paramp, sizeof(int)))
        {
            throw SystemException<EFAULT>();
        }
        argp->s32 = *paramp;
        argp->cls = Param::S32;
        count = argp->s32 * sizeof(wchar_t);
        ++argp;
        ++paramp;
        break;
    case Ent::TypeSequence:
        // int op(xxx* buf, int len, ...);
        if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
        {
            throw SystemException<EFAULT>();
        }
        argp->ptr = ptr = *reinterpret_cast<void**>(paramp);
        argp->cls = Param::PTR;
        ++argp;
        ++paramp;
        if (!isValid(paramp, sizeof(int)))
        {
            throw SystemException<EFAULT>();
        }
        argp->s32 = *paramp;
        argp->cls = Param::S32;
        count = argp->s32 * returnType.getSize();
        ++argp;
        ++paramp;
        break;
    case Ent::SpecUuid:
    case Ent::TypeStructure:
        // void op(struct* buf, ...);
        if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
        {
            throw SystemException<EFAULT>();
        }
        argp->ptr = ptr = *reinterpret_cast<void**>(paramp);
        argp->cls = Param::PTR;
        count = returnType.getSize();
        ++argp;
        ++paramp;
        break;
    case Ent::TypeArray:
        // void op(xxx[x] buf, ...);
        if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
        {
            throw SystemException<EFAULT>();
        }
        argp->ptr = ptr = *reinterpret_cast<void**>(paramp);
        argp->cls = Param::PTR;
        count = returnType.getSize();
        ++argp;
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

        ptr = 0;
        count = 0;

        switch (type.getType())
        {
        case Ent::SpecAny:  // XXX x86 specific
        case Ent::SpecBool:
        case Ent::SpecChar:
        case Ent::SpecWChar:
        case Ent::SpecS8:
        case Ent::SpecS16:
        case Ent::SpecS32:
        case Ent::SpecU8:
        case Ent::SpecU16:
        case Ent::SpecU32:
            if (param.isInput())
            {
                if (!isValid(paramp, sizeof(int)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->s32 = *paramp;
                argp->cls = Param::S32;
                ++paramp;
            }
            else
            {
                if (!isValid(reinterpret_cast<int**>(paramp), sizeof(int**)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->ptr = ptr = *reinterpret_cast<int**>(paramp);
                argp->cls = Param::PTR;
                ++paramp;
            }
            break;
        case Ent::SpecS64:
        case Ent::SpecU64:
            if (param.isInput())
            {
                if (!isValid(reinterpret_cast<long long*>(paramp), sizeof(long long)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->s64 = *reinterpret_cast<long long*>(paramp);
                argp->cls = Param::S64;
                paramp += 2;
            }
            else
            {
                if (!isValid(reinterpret_cast<long long**>(paramp), sizeof(long long**)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->ptr = ptr = *reinterpret_cast<long long**>(paramp);
                argp->cls = Param::PTR;
                ++paramp;
            }
            break;
        case Ent::SpecF32:
            if (param.isInput())
            {
                if (!isValid(reinterpret_cast<float*>(paramp), sizeof(float)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->f32 = *reinterpret_cast<float*>(paramp);
                argp->cls = Param::F32;
                ++paramp;
            }
            else
            {
                if (!isValid(reinterpret_cast<float**>(paramp), sizeof(float**)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->ptr = ptr = *reinterpret_cast<float**>(paramp);
                argp->cls = Param::PTR;
                ++paramp;
            }
            break;
        case Ent::SpecF64:
            if (param.isInput())
            {
                if (!isValid(reinterpret_cast<double*>(paramp), sizeof(double)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->f64 = *reinterpret_cast<double*>(paramp);
                argp->cls = Param::F64;
                paramp += 2;
            }
            else
            {
                if (!isValid(reinterpret_cast<double**>(paramp), sizeof(double**)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->ptr = ptr = *reinterpret_cast<double**>(paramp);
                argp->cls = Param::PTR;
                ++paramp;
            }
            break;
        case Ent::SpecString:
            if (!isValid(reinterpret_cast<char**>(paramp), sizeof(char*)))
            {
                throw SystemException<EFAULT>();
            }
            argp->ptr = ptr = *reinterpret_cast<char**>(paramp);
            argp->cls = Param::PTR;
            ++paramp;
            if (param.isInput())
            {
                count = sizeof(char);       // XXX check string length?
            }
            else
            {
                if (!isValid(paramp, sizeof(int)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->s32 = count = *paramp;
                argp->cls = Param::S32;
                ++paramp;
            }
            break;
        case Ent::SpecWString:
            if (!isValid(reinterpret_cast<wchar_t**>(paramp), sizeof(wchar_t*)))
            {
                throw SystemException<EFAULT>();
            }
            argp->ptr = ptr = *reinterpret_cast<wchar_t**>(paramp);
            argp->cls = Param::PTR;
            ++paramp;
            if (param.isInput())
            {
                count = sizeof(wchar_t);    // XXX check string length?
            }
            else
            {
                if (!isValid(paramp, sizeof(int)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->s32 = *paramp;
                argp->cls = Param::S32;
                count = sizeof(wchar_t) * argp->s32;
                ++paramp;
            }
            break;
        case Ent::TypeSequence:
            // xxx* buf, int len, ...
            if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
            {
                throw SystemException<EFAULT>();
            }
            argp->ptr = ptr = *reinterpret_cast<void**>(paramp);
            argp->cls = Param::PTR;
            ++argp;
            ++paramp;
            if (!isValid(paramp, sizeof(int)))
            {
                throw SystemException<EFAULT>();
            }
            argp->s32 = *paramp;
            argp->cls = Param::S32;
            count = type.getSize() * argp->s32;
            ++paramp;
            break;
        case Ent::SpecUuid:         // Guid* guid, ...
        case Ent::TypeStructure:    // struct* buf, ...
        case Ent::TypeArray:        // xxx[x] buf, ...
            if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
            {
                throw SystemException<EFAULT>();
            }
            argp->ptr = ptr = *reinterpret_cast<void**>(paramp);
            argp->cls = Param::PTR;
            count = type.getSize();
            ++paramp;
            break;
        case Ent::TypeInterface:
            iid = type.getInterface().getIid();
            // FALL THROUGH
        case Ent::SpecObject:
            if (param.isInput())
            {
                if (!isValid(reinterpret_cast<void**>(paramp), sizeof(void*)))
                {
                    throw SystemException<EFAULT>();
                }
                argp->ptr = ptr = *reinterpret_cast<void**>(paramp);
                argp->cls = Param::PTR;
                ++paramp;
                if (void** ip = reinterpret_cast<void**>(ptr))
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
                        argp->ptr = reinterpret_cast<void*>(inputProxies[i]->getObject());
                    }
                    else    // XXX Check range
                    {
                        // Allocate an entry in the upcall table and set the
                        // interface pointer to the broker for the upcall table.
                        int n = set(this, (IInterface*) ip, iid, false);
                        if (n < 0)
                        {
                            throw SystemException<ENFILE>();
                        }
                        // Note the reference count of the created upcall proxy must
                        // be decremented by one at the end of this system call.
                        upcallProxies[i] = &upcallTable[n];
                        argp->ptr = &(broker.getInterfaceTable())[n];
                        if (log)
                        {
                            esReport(" = %p", ip);
                        }
                    }
                }
                ptr = 0;
            }
            else
            {
                // The output interface pointer parameter is no longer supported.
                throw SystemException<EINVAL>();
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

        if (type.getType() == Ent::SpecUuid && param.isInput())
        {
            iid = *static_cast<Guid*>(ptr);
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
    long long rc;
    switch (returnType.getType())
    {
    case Ent::SpecAny:  // XXX x86 specific
    case Ent::SpecBool:
    case Ent::SpecChar:
    case Ent::SpecWChar:
    case Ent::SpecS8:
    case Ent::SpecS16:
    case Ent::SpecS32:
    case Ent::SpecU8:
    case Ent::SpecU16:
    case Ent::SpecU32:
        rc = applyS32(argc, argv, (s32 (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecS64:
    case Ent::SpecU64:
        rc = applyS64(argc, argv, (s64 (*)()) ((*object)[methodNumber]));
        break;
    case Ent::SpecF32:
        applyF32(argc, argv, (f32 (*)()) ((*object)[methodNumber]));    // XXX
        break;
    case Ent::SpecF64:
        applyF64(argc, argv, (f64 (*)()) ((*object)[methodNumber]));    // XXX
        break;
    case Ent::SpecString:
    case Ent::SpecWString:
    case Ent::TypeSequence:
        rc = applyS32(argc, argv, (s32 (*)()) ((*object)[methodNumber]));
        break;
    case Ent::TypeInterface:
        iid = returnType.getInterface().getIid();
        // FALL THROUGH
    case Ent::SpecObject:
        rc = (long) applyPTR(argc, argv, (const void* (*)()) ((*object)[methodNumber]));
        if (void* ip = reinterpret_cast<void*>(rc))
        {
            int n = set(syscallTable, ip, iid, true);
            if (0 <= n)
            {
                rc = reinterpret_cast<long>(&base[n]);
            }
            else
            {
                IInterface* object(static_cast<IInterface*>(ip));
                object->release();
                rc = 0;
                throw SystemException<EMFILE>();
            }
        }
        break;
    case Ent::TypeArray:
    case Ent::SpecVoid:
        applyS32(argc, argv, (s32 (*)()) ((*object)[methodNumber]));
        rc = 0;
        break;
    }

    // Process addRef() and release() locally
    if (interface.getIid() == IMonitor::iid())
    {
        unsigned long count;
        switch (methodNumber)
        {
        case 3: // lock
            count = proxy->addUser();
            break;
        case 4: // tryLock
            if (rc)
            {
                count = proxy->addUser();
            }
            break;
        case 5: // unlock
            count = proxy->releaseUser();
            break;
        }
    }

    return rc;
}
