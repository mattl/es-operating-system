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
#include <new>
#include <es.h>
#include <es/broker.h>
#include <es/exception.h>
#include <es/reflect.h>
#include "core.h"
#include "process.h"

extern Reflect::Interface* getInterface(const Guid* iid);

typedef long long (*Method)(void* self, ...);

Broker<Process::upcall, Process::INTERFACE_POINTER_MAX> Process::broker;
UpcallProxy Process::upcallTable[Process::INTERFACE_POINTER_MAX];

bool UpcallProxy::set(Process* process, void* interface, const Guid* iid)
{
    if (ref.addRef() != 1)
    {
        ref.release();
        return false;
    }
    this->interface = interface;
    this->iid = iid;
    this->process = process;
    use.exchange(0);
    return true;
}

unsigned int UpcallProxy::addRef()
{
    return ref.addRef();
}

unsigned int UpcallProxy::release()
{
    return ref.release();
}

bool UpcallProxy::isUsed()
{
    return (use == 0 && use.increment() == 1) ? false : true;
}

int Process::
set(Process* process, void* interface, const Guid* iid)
{
    for (UpcallProxy* proxy(upcallTable);
         proxy < &upcallTable[INTERFACE_POINTER_MAX];
         ++proxy)
    {
        if (proxy->set(process, interface, iid))
        {
#ifdef VERBOSE
            esReport("Process::set(%p, {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}) : %d;\n",
                     interface,
                     iid->Data1, iid->Data2, iid->Data3,
                     iid->Data4[0], iid->Data4[1], iid->Data4[2], iid->Data4[3],
                     iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7],
                     proxy - table);
#endif
            return proxy - upcallTable;
        }
    }
    return -1;
}

long long Process::
upcall(void* self, void* base, int methodNumber, va_list ap)
{
    Thread* current(Thread::getCurrentThread());
    Core* core(Core::getCurrentCore());

    unsigned interfaceNumber((void**) self - (void**) base);
    UpcallProxy* proxy = &upcallTable[interfaceNumber];

    // Now we need to identify which process is to be used for this upcall.
    Process* server = proxy->process;
#ifdef VERBOSE
    esReport("Process(%p)::upcall[%d] %d\n", server, interfaceNumber, methodNumber);
#endif
    UpcallRecord* record = server->getUpcallRecord();
    if (!record)
    {
        throw SystemException<ENOMEM>();
    }
    bool log(server->log);

    // Determine the type of interface and which method is being invoked.
    Reflect::Interface* interface = getInterface(proxy->iid);
    ASSERT(interface);

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
    record->method = Reflect::Function(super->getMethod(methodNumber - baseMethodCount));

    if (log)
    {
        esReport("upcall: %s::%s(",
                 interface->getName(), record->method.getName());
    }

    unsigned long ref;
    if (*super->getIid() == IID_IInterface)
    {
        switch (methodNumber - baseMethodCount)
        {
        case 1: // addRef
            ref = proxy->addRef();
            // If this is the first addRef() call to proxy,
            // redirect the call to the original object.
            if (proxy->isUsed())
            {
                if (log)
                {
                    esReport(") : %d/%d;\n", ref, (long) proxy->use);
                }
                return ref;
            }
            break;
        case 2: // release
            if (1 < proxy->ref || !proxy->use)
            {
                ref = proxy->release();
                if (log)
                {
                    esReport(") : %d/%d;\n", ref, (long) proxy->use);
                }
                return ref;
            }
            break;
        }
    }

    // Save the upcall context
    record->client = getCurrentProcess();
    record->proxy = proxy;
    record->methodNumber = methodNumber;
    record->param = ap;

    long long result(0);
    switch (record->getState())
    {
    case UpcallRecord::INIT:
        // Leap into the server process.
        current->leapIntoServer(record);
        server->load();

        // Initialize TLS.
        memmove(reinterpret_cast<void*>(record->ureg.esp),
                server->tlsImage, server->tlsImageSize);

        record->push(0);                                            // param
        record->push(reinterpret_cast<unsigned>(server->focus));    // start
        record->push(0);                                            // ret
        record->entry(reinterpret_cast<unsigned>(server->startup));

        if (record->label.set() == 0)
        {
            // Make an upcall the server process.
            record->sp0 = core->tss->sp0;
            core->tss->sp0 = current->sp0 = record->label.esp;
            record->ureg.load();
            // NOT REACHED HERE
        }
        else
        {
            // Switch the record state to READY.
            record->setState(UpcallRecord::READY);

            // Return to the client process.
            Process* client = current->returnToClient();
            if (client)
            {
                client->load();
            }
        }
        // FALL THROUGH
    case UpcallRecord::READY:
        // Copy parameters to the user stack of the server process
        server->copyIn(record);

        // Leap into the server process.
        current->leapIntoServer(record);
        server->load();

        // Invoke method
        record->ureg.eax = reinterpret_cast<u32>(record->proxy->interface);
        record->ureg.edx = record->methodNumber;
#ifdef VERBOSE
        esReport("ureg->eip: %p of %p[%d]\n", ureg->eax, object, record->methodNumber);
#endif
        if (record->label.set() == 0)
        {
            // Make an upcall the server process.
            record->sp0 = core->tss->sp0;
            core->tss->sp0 = current->sp0 = record->label.esp;
            record->ureg.load();
            // NOT REACHED HERE
        }
        else
        {
            // Return to the client process.
            Process* client = current->returnToClient();
            if (client)
            {
                client->load();
            }

            // Copy output parameters from the user stack of the server process.
            server->copyOut(record);

            // Get result code
            result = ((long long) record->ureg.edx << 32) | record->ureg.eax;
            int errorCode = record->ureg.ecx;

            // Process return code
            Reflect::Type returnType(record->method.getReturnType());
            if (errorCode == 0 && returnType.isInterfacePointer())
            {
                // Convert the received interface pointer to kernel's interface pointer
                void** ip((void**) result);
                if (server->ipt <= ip && ip < server->ipt + INTERFACE_POINTER_MAX)
                {
                    unsigned interfaceNumber(ip - server->ipt);
                    SyscallProxy* proxy = &server->syscallTable[interfaceNumber];
                    result = (long long) proxy->interface;
                }
                else
                {
                    // Allocate an entry in the upcall table and set the
                    // interface pointer to the broker for the upcall table.
                    int n = set(server, (IInterface*) ip, returnType.getInterface().getIid());
                    if (log)
                    {
                        esReport(" = %p", ip);
                    }
                    result = (long long) &(broker.getInterfaceTable())[n];
                }
            }

            server->putUpcallRecord(record);

            if (errorCode)
            {
                // Switch the record state back to INIT.
                record->setState(UpcallRecord::INIT);
                esThrow(errorCode);
            }
        }
        break;
    }

    if (*super->getIid() == IID_IInterface)
    {
        switch (methodNumber - baseMethodCount)
        {
        case 1: // addRef
            result = ref;
            break;
        case 2: // release
            result = proxy->release();
            break;
        }
    }

    return result;
}

void Process::
returnFromUpcall(Ureg* ureg)
{
    Thread* current(Thread::getCurrentThread());
    UpcallRecord* record(current->upcallList.getLast());
    if (!record)
    {
        return; // Invalid return from upcall
    }
    ASSERT(record->process == getCurrentProcess());

    Core* core(Core::getCurrentCore());
    core->tss->sp0 = current->sp0 = record->sp0;
    memmove(&record->ureg, ureg, sizeof(Ureg));
    record->label.jump();
    // NOT REACHED HERE
}

void Process::
setFocus(void* (*focus)(void* param))
{
    this->focus = focus;
}

UpcallRecord* Process::
createUpcallRecord(const unsigned stackSize)
{
    // Map a user stack
    void* userStack(static_cast<u8*>(USER_MAX) - ((threadCount + upcallCount + 1) * stackSize));
    userStack = map(userStack, stackSize - Page::SIZE,
                    ICurrentProcess::PROT_READ | ICurrentProcess::PROT_WRITE,
                    ICurrentProcess::MAP_PRIVATE, 0, 0);
    if (!userStack)
    {
        return 0;
    }

    UpcallRecord* record = new(std::nothrow) UpcallRecord(this);
    if (!record)
    {
        unmap(userStack, stackSize - Page::SIZE);
        return 0;
    }

    Ureg* ureg = &record->ureg;
    memset(ureg, 0, sizeof(Ureg));
    ureg->gs = Core::TCBSEL;
    ureg->fs = ureg->es = ureg->ds = ureg->ss = Core::UDATASEL;
    ureg->cs = Core::UCODESEL;
    ureg->eflags = 0x0202;  // IF
    ureg->esp = reinterpret_cast<unsigned>(static_cast<u8*>(userStack) + stackSize - Page::SIZE);
    record->tls(tlsSize, tlsAlign);
    record->userStack = userStack;

    ++upcallCount;

    return record;
}

UpcallRecord* Process::
getUpcallRecord()
{
    UpcallRecord* record(upcallList.removeFirst());
    if (record)
    {
        return record;
    }
    const unsigned stackSize = 2*1024*1024;
    return createUpcallRecord(stackSize);
}

void Process::
putUpcallRecord(UpcallRecord* record)
{
    upcallList.addLast(record);
}

// Note copyIn() is called against the server process, so that
// copyIn() can be called from the kernel thread to make an upcall,
void Process::
copyIn(UpcallRecord* record)
{
    UpcallProxy* proxy(record->proxy);
    int methodNumber(record->methodNumber);
    void* paramv(record->param);
    u8* esp(reinterpret_cast<u8*>(record->ureg.esp));
    Ureg* ureg(&record->ureg);

    //
    // Determine the type of interface and which method is being invoked.
    //
    Reflect::Interface* interface = getInterface(proxy->iid);
    ASSERT(interface);

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

    //
    // Copy parameters
    //
    int paramc(0);
    unsigned param[8];  // XXX 8 is enough?
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
        if (parameter.isInterfacePointer() && !parameter.isOutput())
        {
            void** ip(*(void***) (param + paramc));

            const Guid* iid;
            if (0 <= parameter.getIidIs())
            {
                iid = *(Guid**) ((u8*) paramv + method.getParameterOffset(parameter.getIidIs()));
            }
            else
            {
                iid = parameter.getType().getInterface().getIid();
            }

            // Set up new interface proxy.
            // We also need to know the IID of the interface.
            if (ip)
            {
                IInterface* object((IInterface*) ip);

                int n = set(syscallTable, object, iid);
                if (0 <= n)
                {
                    // Set ip to proxy ip
                    ip = &ipt[n];
                    object->addRef();
                }
                else
                {
                    ip = 0;
                }
            }
            *(void***) (param + paramc) = ((void***) paramv)[paramc] = ip;

            // Note the reference count to the created syscall proxy must
            // be decremented by one at the end of this upcall.
        }
        else if (parameter.getType().isPointer() || parameter.getType().isReference())
        {
            if (parameter.isInput() || parameter.isOutput())
            {
                // Determine the size of the object pointed.
                int count(0);
                if (0 <= parameter.getSizeIs())
                {
                    count = *(int*) ((u8*) paramv + method.getParameterOffset(parameter.getSizeIs()));
                }
                else
                {
                    // Determine the size of object pointed by this parameter.
                    count = parameter.getType().getReferentSize();
                }

                if (Page::SIZE < count)
                {
                    throw SystemException<ENOBUFS>();
                }

                // Reserve count space in the server user stack.
                esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
                if (parameter.isInput())
                {
                    write(((void**) paramv)[paramc], count, reinterpret_cast<long long>(esp));
                }
                *(void**) (param + paramc) = esp;
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

    // XXX
    // Note the following stage would be much easier to implement if the
    // current process is the server process itself.

    // Copy arguments
    esp -= sizeof(int) * paramc;
    write(param, sizeof(int) * paramc, reinterpret_cast<long long>(esp));

    // Copy 'this'
    esp -= sizeof(int);
    write(&proxy->interface, sizeof(int), reinterpret_cast<long long>(esp));

    ureg->esp = reinterpret_cast<u32>(esp);
}

// Note copyIn() is called against the server process.
void Process::
copyOut(UpcallRecord* record)
{
    UpcallProxy* proxy(record->proxy);
    int methodNumber(record->methodNumber);
    void* paramv(record->param);
    u8* esp(reinterpret_cast<u8*>(record->ureg.esp));  // XXX should be saved separately

    //
    // Determine the type of interface and which method is being invoked.
    //
    Reflect::Interface* interface = getInterface(proxy->iid);
    ASSERT(interface);

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
        esReport("return from upcall: %s::%s(",
                 interface->getName(), method.getName());
    }

    //
    // Copy parameters
    //
    int paramc(0);
    unsigned param[8];  // XXX 8 is enough?
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

        if (parameter.isInterfacePointer() && !parameter.isOutput())
        {
            void** ip(((void***) paramv)[paramc]);
            if (log)
            {
                esReport(" = %p", ip);
            }
            if (ip)
            {
                // Release the created syscall proxy.
                SyscallProxy* proxy(&syscallTable[ip - ipt]);
                proxy->release();
            }
        }
        else if (parameter.getType().isPointer() || parameter.getType().isReference())
        {
            if (parameter.isInput() || parameter.isOutput())
            {
                // Determine the size of the object pointed.
                int count(0);
                if (0 <= parameter.getSizeIs())
                {
                    count = *(int*) ((u8*) paramv + method.getParameterOffset(parameter.getSizeIs()));
                }
                else
                {
                    // Determine the size of object pointed by this parameter.
                    count = parameter.getType().getReferentSize();
                }

                if (Page::SIZE < count)
                {
                    throw SystemException<ENOBUFS>();
                }

                // Reserve count space in the server user stack.
                esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
                if (parameter.isOutput())
                {
                    read(((void**) paramv)[paramc], count, reinterpret_cast<long long>(esp));
                }
            }
        }

        if (parameter.isInterfacePointer())
        {
            if (parameter.isOutput())
            {
                // Convert the received interface pointer to kernel's interface pointer
                void** ip(*(((void****) paramv)[paramc]));

                if (ipt <= ip && ip < ipt + INTERFACE_POINTER_MAX)
                {
                    SyscallProxy* proxy = &syscallTable[ip - ipt];
                    *((void***) paramv)[paramc] = (void**) proxy->interface;
                }
                else if (ip)
                {
                    // Allocate an entry in the upcall table and set the
                    // interface pointer to the broker for the upcall table.
                    const Guid* iid;
                    if (0 <= parameter.getIidIs())
                    {
                        iid = *(Guid**) ((u8*) paramv + method.getParameterOffset(parameter.getIidIs()));
                    }
                    else
                    {
                        iid = parameter.getType().getInterface().getIid();
                    }

                    int n = set(this, (IInterface*) ip, iid);
                    if (log)
                    {
                        esReport(" = %p", ip);
                    }
                    *((void***) paramv)[paramc] = &(broker.getInterfaceTable())[n];
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
}
