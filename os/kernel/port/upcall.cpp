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
#include <es/handle.h>
#include <es/reflect.h>
#include "core.h"
#include "interfaceStore.h"
#include "process.h"

typedef long long (*Method)(void* self, ...);

Broker<Process::upcall, Process::INTERFACE_POINTER_MAX> Process::broker;
UpcallProxy Process::upcallTable[Process::INTERFACE_POINTER_MAX];

bool UpcallProxy::set(Process* process, void* object, const Guid& iid)
{
    if (ref.addRef() != 1)
    {
        ref.release();
        return false;
    }
    this->object = object;
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
set(Process* process, void* object, const Guid& iid)
{
    for (UpcallProxy* proxy(upcallTable);
         proxy < &upcallTable[INTERFACE_POINTER_MAX];
         ++proxy)
    {
        if (proxy->set(process, object, iid))
        {
#ifdef VERBOSE
            esReport("Process::set(%p, {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}) : %d;\n",
                     object,
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

    unsigned interfaceNumber(static_cast<void**>(self) - static_cast<void**>(base));
    UpcallProxy* proxy = &upcallTable[interfaceNumber];

    // Now we need to identify which process is to be used for this upcall.
    Process* server = proxy->process;
#ifdef VERBOSE
    esReport("Process(%p)::upcall[%d] %d\n", server, interfaceNumber, methodNumber);
#endif
    bool log(server->log);

    // Determine the type of interface and which method is being invoked.
    Reflect::Interface interface = getInterface(proxy->iid);   // XXX Should cache the result

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
    Reflect::Function method(Reflect::Function(super.getMethod(methodNumber - baseMethodCount)));

    if (log)
    {
        esReport("upcall[%d:%p]: %s::%s(",
                 interfaceNumber, server, interface.getName(), method.getName());
    }

    unsigned long ref;
    if (super.getIid() == IID_IInterface)
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

    // Get a free upcall record
    UpcallRecord* record = server->getUpcallRecord();
    if (!record)
    {
        throw SystemException<ENOMEM>();
    }
    record->method = method;

    // Save the upcall context
    record->client = getCurrentProcess();
    record->proxy = proxy;
    record->methodNumber = methodNumber;
    record->param = ap;

    long long result(0);
    int errorCode(0);
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
            unsigned x = Core::splHi();
            Core* core = Core::getCurrentCore();
            record->sp0 = core->tss->sp0;
            core->tss->sp0 = current->sp0 = record->label.esp;
            Core::splX(x);
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
        errorCode = server->copyIn(record);
        if (errorCode)
        {
            break;
        }

        // Leap into the server process.
        current->leapIntoServer(record);
        server->load();

        // Invoke method
        record->ureg.eax = reinterpret_cast<u32>(record->proxy->object);
        record->ureg.edx = record->methodNumber;
#ifdef VERBOSE
        esReport("ureg->eip: %p of %p[%d]\n", ureg->eax, object, record->methodNumber);
#endif
        if (record->label.set() == 0)
        {
            // Make an upcall the server process.
            unsigned x = Core::splHi();
            Core* core = Core::getCurrentCore();
            record->sp0 = core->tss->sp0;
            core->tss->sp0 = current->sp0 = record->label.esp;
            Core::splX(x);
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
            errorCode = server->copyOut(record);

            // Get result code
            if (errorCode == 0)
            {
                result = (static_cast<long long>(record->ureg.edx) << 32) | record->ureg.eax;
                errorCode = record->ureg.ecx;
            }

            // Process return code
            Reflect::Type returnType(record->method.getReturnType());
            if (errorCode == 0 && returnType.isInterfacePointer())
            {
                // Convert the received interface pointer to kernel's interface pointer
                void** ip(reinterpret_cast<void**>(result));
                if (server->ipt <= ip && ip < server->ipt + INTERFACE_POINTER_MAX)
                {
                    unsigned interfaceNumber(ip - server->ipt);
                    Handle<SyscallProxy> proxy(&server->syscallTable[interfaceNumber], true);
                    if (proxy->isValid())
                    {
                        result = reinterpret_cast<long>(proxy->getObject());
                    }
                    else
                    {
                        result = 0;
                        errorCode = EBADFD;
                    }
                }
                else if (server->isValid(ip, sizeof(void*)))
                {
                    // Allocate an entry in the upcall table and set the
                    // interface pointer to the broker for the upcall table.
                    int n = set(server, reinterpret_cast<IInterface*>(ip), returnType.getInterface().getIid());
                    if (0 <= n)
                    {
                        result = reinterpret_cast<long>(&(broker.getInterfaceTable())[n]);
                    }
                    else
                    {
                        // XXX object pointed by ip would be left allocated.
                        result = 0;
                        errorCode = ENFILE;
                    }
                    if (log)
                    {
                        esReport(" = %p", ip);
                    }
                }
                else
                {
                    errorCode = EBADFD;
                }
            }
        }
        break;
    }

    if (super.getIid() == IID_IInterface)
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

    if (errorCode)
    {
        // Switch the record state back to INIT.
        record->setState(UpcallRecord::INIT);
        server->putUpcallRecord(record);
        esThrow(errorCode);
    }

    server->putUpcallRecord(record);

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

    unsigned x = Core::splHi();
    Core* core = Core::getCurrentCore();
    core->tss->sp0 = current->sp0 = record->sp0;
    Core::splX(x);
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

    upcallCount.increment();

    return record;
}

UpcallRecord* Process::
getUpcallRecord()
{
    {
        Lock::Synchronized method(spinLock);

        UpcallRecord* record = upcallList.removeFirst();
        if (record)
        {
            return record;
        }
    }

    const unsigned stackSize = 2*1024*1024;
    return createUpcallRecord(stackSize);
}

void Process::
putUpcallRecord(UpcallRecord* record)
{
    Lock::Synchronized method(spinLock);

    upcallList.addLast(record);
}

// Note copyIn() is called against the server process, so that
// copyIn() can be called from the kernel thread to make an upcall,
int Process::
copyIn(UpcallRecord* record)
{
    UpcallProxy* proxy(record->proxy);
    int methodNumber(record->methodNumber);
    va_list paramv(record->param);
    u8* esp(reinterpret_cast<u8*>(record->ureg.esp));
    Ureg* ureg(&record->ureg);

    unsigned arg[sizeof(void*) / sizeof(unsigned) + 8]; // XXX 8 is enough?
    int paramc(0);
    unsigned* param(arg + sizeof(void*) / sizeof(unsigned));
    for (int i(0); i < record->method.getParameterCount(); ++i)
    {
        Reflect::Identifier parameter(record->method.getParameter(i));
        if (log)
        {
            esReport("%s", parameter.getName());
        }
        int size = parameter.getType().getSize();
        size += sizeof(unsigned) - 1;
        size &= ~(sizeof(unsigned) - 1);
        memmove(&param[paramc], &reinterpret_cast<int*>(paramv)[paramc], size);
        if (parameter.isInterfacePointer() && !parameter.isOutput())
        {
            void** ip(*reinterpret_cast<void***>(param + paramc));
            if (ip)
            {
                // Check the IID of the interface pointer.
                Guid iid;
                if (0 <= parameter.getIidIs())
                {
                    iid = **reinterpret_cast<Guid**>(reinterpret_cast<u8*>(paramv) + record->method.getParameterOffset(parameter.getIidIs()));
                    Reflect::Interface interface;
                    try
                    {
                        interface = getInterface(iid);
                    }
                    catch (Exception& error)
                    {
                        return error.getResult();
                    }
                    iid = interface.getIid();
                }
                else
                {
                    iid = parameter.getType().getInterface().getIid();
                }

                // Set up a new system call proxy.
                IInterface* object(reinterpret_cast<IInterface*>(ip));
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
                *reinterpret_cast<void***>(param + paramc) = *reinterpret_cast<void***>(reinterpret_cast<int*>(paramv) + paramc) = ip;
            }

            // Note the reference count to the created syscall proxy must
            // be decremented by one at the end of this upcall.
        }
        else if (parameter.getType().isPointer() || parameter.getType().isReference())
        {
            // Determine the size of the object pointed.
            int count(0);
            if (0 <= parameter.getSizeIs())
            {
                count = *reinterpret_cast<int*>(reinterpret_cast<u8*>(paramv) + record->method.getParameterOffset(parameter.getSizeIs()));
            }
            else if (parameter.getType().isString())
            {
                // Check zero termination
                char* ptr = *reinterpret_cast<char**>(&param[paramc]);
                for (count = 1; *ptr != '\0'; ++count)
                {
                    ++ptr;
                    if (!isValid(ptr, 1))
                    {
                        return EFAULT;
                    }
                }
                if (log)
                {
                    esReport("[count = %d]", count);
                }
            }
            else
            {
                count = parameter.getType().getReferentSize();
            }

            if (Page::SIZE < count)
            {
                return ENOBUFS;
            }

            // Reserve count space in the server user stack.
            esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
            if (!parameter.isOutput())
            {
                write(*reinterpret_cast<void**>(reinterpret_cast<int*>(paramv) + paramc), count, reinterpret_cast<long long>(esp));
            }
            *reinterpret_cast<void**>(param + paramc) = esp;
        }
        paramc += size / sizeof(unsigned);
        ASSERT(paramc <= 8);    // XXX
        if (log && i + 1 < record->method.getParameterCount())
        {
            esReport(", ");
        }
    }
    if (log)
    {
        esReport(");\n");
    }

    // Copy arguments with 'this' pointer
    memmove(arg, &proxy->object, sizeof(void*));
    esp -= sizeof(void*) + sizeof(int) * paramc;
    write(arg, sizeof(void*) + sizeof(int) * paramc, reinterpret_cast<long>(esp));
    ureg->esp = reinterpret_cast<long>(esp);

    return 0;
}

// Note copyOut() is called against the server process.
int Process::
copyOut(UpcallRecord* record)
{
    UpcallProxy* proxy(record->proxy);
    int methodNumber(record->methodNumber);
    void* paramv(record->param);
    u8* esp(reinterpret_cast<u8*>(record->ureg.esp));  // XXX should be saved separately

    if (log)
    {
        Reflect::Interface interface;
        try
        {
            interface = getInterface(proxy->iid);
        }
        catch (Exception& error)
        {
            return error.getResult();
        }
        esReport("return from upcall: %s::%s(",
                 interface.getName(), record->method.getName());
    }

    int paramc(0);
    for (int i(0); i < record->method.getParameterCount(); ++i)
    {
        Reflect::Identifier parameter(record->method.getParameter(i));
        if (log)
        {
            esReport("%s", parameter.getName());
        }
        int size = parameter.getType().getSize();
        size += sizeof(unsigned) - 1;
        size &= ~(sizeof(unsigned) - 1);

        if (parameter.isInterfacePointer() && !parameter.isOutput())
        {
            void** ip(*reinterpret_cast<void***>(reinterpret_cast<int*>(paramv) + paramc));
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
            // Determine the size of the object pointed.
            int count(0);
            if (0 <= parameter.getSizeIs())
            {
                count = *reinterpret_cast<int*>(reinterpret_cast<u8*>(paramv) + record->method.getParameterOffset(parameter.getSizeIs()));
            }
            else
            {
                count = parameter.getType().getReferentSize();
            }

            if (Page::SIZE < count)
            {
                return ENOBUFS;
            }

            // Reserve count space in the server user stack.
            esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
            if (parameter.isOutput())
            {
                read(*reinterpret_cast<void**>(reinterpret_cast<int*>(paramv) + paramc), count, reinterpret_cast<long long>(esp));
            }
        }

        if (parameter.isInterfacePointer())
        {
            if (parameter.isOutput())
            {
                // Convert the received interface pointer to kernel's interface pointer
                void** ip(**reinterpret_cast<void****>(reinterpret_cast<int*>(paramv) + paramc));

                if (ipt <= ip && ip < ipt + INTERFACE_POINTER_MAX)
                {
                    Handle<SyscallProxy> proxy(&syscallTable[ip - ipt], true);
                    if (proxy->isValid())
                    {
                        **reinterpret_cast<void****>(reinterpret_cast<int*>(paramv) + paramc) = static_cast<void**>(proxy->getObject());
                    }
                    else
                    {
                        **reinterpret_cast<void****>(reinterpret_cast<int*>(paramv) + paramc) = 0;
                        return EBADFD;
                    }
                }
                else if (isValid(ip, sizeof(void*)))
                {
                    // Allocate an entry in the upcall table and set the
                    // interface pointer to the broker for the upcall table.
                    Guid iid;
                    if (0 <= parameter.getIidIs())
                    {
                        iid = **reinterpret_cast<Guid**>(reinterpret_cast<u8*>(paramv) + record->method.getParameterOffset(parameter.getIidIs()));
                        Reflect::Interface interface;
                        try
                        {
                            interface = getInterface(iid);
                        }
                        catch (Exception& error)
                        {
                            return error.getResult();
                        }
                        iid = interface.getIid();
                    }
                    else
                    {
                        iid = parameter.getType().getInterface().getIid();
                    }

                    int n = set(this, reinterpret_cast<IInterface*>(ip), iid);
                    if (0 <= n)
                    {
                        **reinterpret_cast<void****>(reinterpret_cast<int*>(paramv) + paramc) = &(broker.getInterfaceTable())[n];
                    }
                    else
                    {
                        // XXX object pointed by ip would be left allocated.
                        **reinterpret_cast<void****>(reinterpret_cast<int*>(paramv) + paramc) = 0;
                        return ENFILE;
                    }
                    if (log)
                    {
                        esReport(" = %p", ip);
                    }
                }
                else
                {
                    return EFAULT;
                }
            }
        }

        paramc += size / sizeof(unsigned);
        ASSERT(paramc <= 8);    // XXX
        if (log && i + 1 < record->method.getParameterCount())
        {
            esReport(", ");
        }
    }
    if (log)
    {
        esReport(");\n");
    }

    return 0;
}
