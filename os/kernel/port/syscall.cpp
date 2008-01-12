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

#include <errno.h>
#include <stddef.h>
#include <es.h>
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

bool SyscallProxy::set(void* object, const Guid& iid)
{
    if (ref.addRef() != 1)
    {
        ref.release();
        return false;
    }
    this->object = object;
    this->iid = iid;
    use.exchange(1);
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
        object->release();
        object = 0;
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

    // If this interface inherits another interface,
    // methodNumber is checked accordingly.
    if (interface.getTotalMethodCount() <= methodNumber)
    {
        throw SystemException<ENOSYS>();
    }
    unsigned baseMethodCount;
    Reflect::Interface super(interface);
    for (;;)
    {
        baseMethodCount = super.getTotalMethodCount() - super.getMethodCount();
        if (baseMethodCount <= methodNumber)
        {
            break;
        }
        super = getInterface(super.getSuperIid());
    }
    Reflect::Function method(super.getMethod(methodNumber - baseMethodCount));

    if (log)
    {
        esReport("system call[%d:%p]: %s::%s(",
                 interfaceNumber, this, interface.getName(), method.getName());
    }

    // Process addRef() and release() locally
    if (super.getIid() == IID_IInterface)
    {
        unsigned long count;
        switch (methodNumber - baseMethodCount)
        {
        case 1: // addRef
            count = proxy->addUser();
            if (log)
            {
                esReport(") : %d;\n", count);
            }
            return count;
            break;
        case 2: // release
            count = proxy->releaseUser();
            if (log)
            {
                esReport(") : %d;\n", count);
            }
            return count;
            break;
        }
    }

    //
    // Copy parameters in
    //
    int paramc(0);
    int param[8];   // XXX 8 is enough?
    struct
    {
        void* object;
        Guid  iid;
    } ipv[8];
    Handle<SyscallProxy> inputProxies[8];
    Handle<SyscallProxy> outputProxies[8];
    Handle<UpcallProxy>  upcallProxies[8];
    for (int i(0); i < method.getParameterCount(); ++i)
    {
        Reflect::Identifier parameter(method.getParameter(i));
        if (log)
        {
            esReport("%s", parameter.getName());
        }
        int size = parameter.getType().getSize();
        size += sizeof(unsigned) - 1;
        size &= ~(sizeof(unsigned) - 1);
        if (!isValid(&reinterpret_cast<int*>(paramv)[paramc], size))
        {
            throw SystemException<EFAULT>();
        }
        memmove(&param[paramc], &reinterpret_cast<int*>(paramv)[paramc], size);

        if (parameter.getType().isPointer() || parameter.getType().isReference())
        {
            if (parameter.isInput() || parameter.isOutput())
            {
                // Determine the size of the object pointed.
                int count(0);
                if (0 <= parameter.getSizeIs())
                {
                    count = *reinterpret_cast<int*>((reinterpret_cast<u8*>(paramv) + method.getParameterOffset(parameter.getSizeIs())));
                }
                else
                {
                    // Determine the size of object pointed by this parameter.
                    count = parameter.getType().getReferentSize();
                }

                // Check range
                void* ptr(*reinterpret_cast<void**>(param + paramc));
                if (ptr && !isValid(ptr, count))
                {
                    if (log)
                    {
                        esReport("<Bad address (%p)>", ((void**) param)[paramc]);
                    }
                    throw SystemException<EFAULT>();
                }
            }
        }

        if (parameter.isInterfacePointer())
        {
            if (parameter.isOutput())
            {
                *reinterpret_cast<void***>(param + paramc) = &ipv[i].object;

                // Check the IID of the interface pointer.
                Guid iid;
                if (0 <= parameter.getIidIs())
                {
                    iid = **reinterpret_cast<Guid**>(reinterpret_cast<u8*>(paramv) + method.getParameterOffset(parameter.getIidIs()));
                    Reflect::Interface interface(getInterface(iid));
                    iid = interface.getIid();
                }
                else
                {
                    ASSERT(parameter.getType().isInterfacePointer());
                    iid = parameter.getType().getInterface().getIid();
                }
                ipv[i].iid = iid;
            }
            else
            {
                void** ip(*reinterpret_cast<void***>(param + paramc));
                if (ip)
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
                        *reinterpret_cast<void***>(param + paramc) = reinterpret_cast<void**>(inputProxies[i]->getObject());
                    }
                    else if (isValid(ip, sizeof(void*)))
                    {
                        // Allocate an entry in the upcall table and set the
                        // interface pointer to the broker for the upcall table.
                        int n = set(this, (IInterface*) ip, parameter.getType().getInterface().getIid());
                        if (n < 0)
                        {
                            throw SystemException<ENFILE>();
                        }
                        // Note the reference count of the created upcall proxy must
                        // be decremented by one at the end of this system call.
                        upcallProxies[i] = &upcallTable[n];
                        *reinterpret_cast<void***>(param + paramc) = &(broker.getInterfaceTable())[n];
                        if (log)
                        {
                            esReport(" = %p", ip);
                        }
                    }
                    else
                    {
                        throw SystemException<EFAULT>();
                    }
                }

            }
        }
        paramc += size / sizeof(int);
        ASSERT(paramc <= 8);    // XXX
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
    long long rc;
    Method** object = reinterpret_cast<Method**>(proxy->getObject());
    switch (paramc)
    {
    case 0:
        rc = (*object)[methodNumber](object);
        break;
    case 1:
        rc = (*object)[methodNumber](object, param[0]);
        break;
    case 2:
        rc = (*object)[methodNumber](object, param[0], param[1]);
        break;
    case 3:
        rc = (*object)[methodNumber](object, param[0], param[1], param[2]);
        break;
    case 4:
        rc = (*object)[methodNumber](object, param[0], param[1], param[2], param[3]);
        break;
    case 5:
        rc = (*object)[methodNumber](object, param[0], param[1], param[2], param[3], param[4]);
        break;
    case 6:
        rc = (*object)[methodNumber](object, param[0], param[1], param[2], param[3], param[4], param[5]);
        break;
    case 7:
        rc = (*object)[methodNumber](object, param[0], param[1], param[2], param[3], param[4], param[5], param[6]);
        break;
    case 8:
        rc = (*object)[methodNumber](object, param[0], param[1], param[2], param[3], param[4], param[5], param[6], param[7]);
        break;
    }

    // Pass interface pointers
    paramc = 0;
    for (int i(0); i < method.getParameterCount(); ++i)
    {
        Reflect::Identifier parameter(method.getParameter(i));
        int size = parameter.getType().getSize();
        size += sizeof(int) - 1;
        size &= ~(sizeof(int) - 1);
        if (parameter.isInterfacePointer())
        {
            if (parameter.isOutput())
            {
                ASSERT(*reinterpret_cast<void***>(param + paramc) == &ipv[i].object);
                void* ip(ipv[i].object);
                if (ip)
                {
                    // Set up new system call proxy.
                    int n = set(syscallTable, static_cast<IInterface*>(ip), ipv[i].iid);
                    if (0 <= n)
                    {
                        // Set ip to proxy ip
                        ip = &base[n];
                        outputProxies[i] = &syscallTable[n];
                    }
                    else
                    {
                        IInterface* object(static_cast<IInterface*>(ip));
                        object->release();
                        ip = 0;
                        throw SystemException<EMFILE>();
                    }
                }
                **reinterpret_cast<void***>(reinterpret_cast<int*>(paramv) + paramc) = ip;
            }
        }
        paramc += size / sizeof(int);
        ASSERT(paramc <= 8);    // XXX
    }

    // Process return code
    Reflect::Type returnType(method.getReturnType());
    if (returnType.isInterfacePointer())
    {
        void* ip(reinterpret_cast<void*>(rc));
        if (ip)
        {
            int n = set(syscallTable, ip, returnType.getInterface().getIid());
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
    }

    // Commit syscallTable modification
    for (int i(0); i < method.getParameterCount(); ++i)
    {
        if (outputProxies[i])
        {
            IInterface* object(*reinterpret_cast<IInterface**>(&outputProxies[i]));
            object->addRef();
        }
    }

    return rc;
}
