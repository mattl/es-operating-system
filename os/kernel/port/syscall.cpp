/*
 * Copyright (c) 2006
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
#include <es/reflect.h>
#include "core.h"
#include "process.h"

extern IStream* esReportStream();
extern Reflect::Interface* getInterface(const Guid* iid);

typedef long long (*Method)(void* self, ...);

bool SyscallProxy::set(void* interface, const Guid* iid)
{
    if (ref.addRef() != 1)
    {
        ref.release();
        return false;
    }
    this->interface = interface;
    this->iid = iid;
    return true;
}

unsigned int SyscallProxy::addRef()
{
    return ref.addRef();
}

unsigned int SyscallProxy::release()
{
    IInterface* object(static_cast<IInterface*>(interface));
    unsigned int count = ref.release();
    if (count == 0)
    {
        object->release();
    }
}

long long Process::
systemCall(void** self, unsigned methodNumber, va_list paramv, void** base)
{
    if (base)
    {
        ipt = base;
    }

    //
    // Determine the type of interface and which method is being invoked.
    //
    unsigned interfaceNumber(self - base);
    if (INTERFACE_POINTER_MAX <= interfaceNumber)
    {
        throw SystemException<EBADF>();
    }
    SyscallProxy* proxy = &syscallTable[interfaceNumber];
    if (proxy->interface == 0)
    {
        throw SystemException<EBADF>();
    }

    Reflect::Interface* interface = getInterface(proxy->iid);
    if (!interface)
    {
        throw SystemException<EBADF>();
    }

    if ((void*) proxy->interface == (void*) esReportStream() ||
        interfaceNumber == 0 && methodNumber == 15)
    {
        log = false;
    }

    // If this interface inherits another interface,
    // methodNumber is checked accordingly.
    if (interface->getTotalMethodCount() <= methodNumber)
    {
        throw SystemException<ENOSYS>();
    }
    unsigned baseMethodCount;
    Reflect::Interface* super(interface);
    for (;;)
    {
        baseMethodCount = super->getTotalMethodCount() - super->getMethodCount();
        if (baseMethodCount <= methodNumber)
        {
            break;
        }
        else
        {
            Guid* piid = super->getSuperIid();
            ASSERT(piid);
            super = getInterface(piid);
        }
    }
    Reflect::Function method(super->getMethod(methodNumber - baseMethodCount));

    if (log)
    {
        esReport("system call[%d:%p]: %s::%s(",
                 interfaceNumber, this, interface->getName(), method.getName());
    }

    // Process addRef() and release() locally
    if (*super->getIid() == IID_IInterface)
    {
        unsigned long count;
        switch (methodNumber - baseMethodCount)
        {
        case 1: // addRef
            count = proxy->addRef();
            if (log)
            {
                esReport(") : %d;\n", count);
            }
            return count;
            break;
        case 2: // release
            count = proxy->release();
            if (log)
            {
                esReport(") : %d;\n", count);
            }
            return count;
            break;
        }
    }

    //
    // Copy parameters
    //
    int paramc(0);
    unsigned param[8];  // XXX 8 is enough?
    void* ipv[8];       // interface pointer vector
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
        memmove(&param[paramc], &((u32*) paramv)[paramc], size);
        if (parameter.isInterfacePointer())
        {
            if (parameter.isOutput())
            {
                *(void***) (param + paramc) = &ipv[paramc];
            }
            else
            {
                void** ip(*(void***) (param + paramc));

                if (base <= ip && ip < base + INTERFACE_POINTER_MAX)
                {
                    unsigned interfaceNumber(ip - base);
                    SyscallProxy* proxy = &syscallTable[interfaceNumber];
                    *(void***) (param + paramc) = (void**) proxy->interface;
                }
                else if (ip)
                {
                    // Allocate an entry in the upcall table and set the
                    // interface pointer to the broker for the upcall table.
                    int n = set(this, (IInterface*) ip, parameter.getType().getInterface().getIid());
                    if (log)
                    {
                        esReport(" = %p", ip);
                    }
                    *(void***) (param + paramc) = &(broker.getInterfaceTable())[n];

                    // Note the reference count to the created upcall proxy must
                    // be decremented by one at the end of this system call.
                }
            }
        }
        paramc += size / sizeof(unsigned);
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
    Method** object = reinterpret_cast<Method**>(proxy->interface);
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
        size += sizeof(unsigned) - 1;
        size &= ~(sizeof(unsigned) - 1);
        if (parameter.isInterfacePointer())
        {
            if (parameter.isOutput())
            {
                ASSERT(*(void***) (param + paramc) == &ipv[paramc]);

                int n(-1);
                void* ip(ipv[paramc]);
                if (ip)
                {
                    // Set up new interface proxy.
                    // We also need to know the IID of the interface.
                    Guid* iid;
                    if (0 <= parameter.getIidIs())
                    {
                        iid = *(Guid**) ((u8*) paramv + method.getParameterOffset(parameter.getIidIs()));
                    }
                    else if (parameter.getType().isInterfacePointer())
                    {
                        iid = parameter.getType().getInterface().getIid();
                    }
                    n = set(syscallTable, (IInterface*) ip, iid);
                }
                if (0 <= n)
                {
                    // Set ip to proxy ip
                    ip = &base[n];
                }
                else
                {
                    // XXX ip->release()
                    ip = 0;
                }
                **(void***) ((u32*) paramv + paramc) = ip;
            }
            else
            {
                void** ip(*(void***) (param + paramc));
                void** base(broker.getInterfaceTable());
                if (base <= ip && ip < base + INTERFACE_POINTER_MAX)
                {
                    unsigned interfaceNumber(ip - base);
                    UpcallProxy* proxy(&upcallTable[interfaceNumber]);
                    proxy->release();
                }
            }
        }
        paramc += size / sizeof(unsigned);
        ASSERT(paramc <= 8);    // XXX
    }

    // Process return code
    Reflect::Type returnType(method.getReturnType());
    if (returnType.isInterfacePointer())
    {
        int n(-1);
        void* ip((void*) rc);
        if (ip)
        {
            n = set(syscallTable, ip, returnType.getInterface().getIid());
        }
        if (0 <= n)
        {
            rc = (long long)(&base[n]);
        }
        else
        {
            // XXX if (ip) { ip->release(); }
            rc = 0;
        }
    }

    return rc;
}
