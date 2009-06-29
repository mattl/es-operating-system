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
#include <string.h>
#include <new>
#include <es.h>
#include <es/any.h>
#include <es/broker.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/reflect.h>
#include "core.h"
#include "interfaceStore.h"
#include "process.h"

// #define VERBOSE

typedef long long (*Method)(void* self, ...);

Broker<Process::upcall, Process::INTERFACE_POINTER_MAX> Process::broker;
UpcallProxy Process::upcallTable[Process::INTERFACE_POINTER_MAX];

bool UpcallProxy::set(Process* process, void* object, const char* iid, bool used)
{
    if (ref.addRef() != 1)
    {
        ref.release();
        return false;
    }
    this->object = object;
    this->iid = es::getUniqueIdentifier(iid);
    this->process = process;
    use.exchange(used ? 1 : 0);
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
set(Process* process, void* object, const char* iid, bool used)
{
    for (UpcallProxy* proxy(upcallTable);
         proxy < &upcallTable[INTERFACE_POINTER_MAX];
         ++proxy)
    {
        if (proxy->set(process, object, iid, used))
        {
#ifdef VERBOSE
            esReport("Process::set(%p, %s) : %d;\n",
                     object, iid, proxy - upcallTable);
#endif
            return proxy - upcallTable;
        }
    }
    return -1;
}

long long Process::
upcall(void* self, void* base, int methodNumber, va_list ap)
{
    Thread* current = Thread::getCurrentThread();

    unsigned interfaceNumber = static_cast<void**>(self) - static_cast<void**>(base);
    Any* variant = 0;
    if (INTERFACE_POINTER_MAX <= interfaceNumber)
    {
        // self must be a pointer to a Any value for the method returns a Any.
        variant = reinterpret_cast<Any*>(self);
        self = va_arg(ap, void*);
        interfaceNumber = static_cast<void**>(self) - static_cast<void**>(base);
    }

    UpcallProxy* proxy = &upcallTable[interfaceNumber];

    // Now we need to identify which process is to be used for this upcall.
    Process* server = proxy->process;
    bool log = server->log;

#ifdef VERBOSE
    esReport("Process(%p)::upcall[%d] %d - %s\n", server, interfaceNumber, methodNumber, proxy->iid);
#endif

    // Determine the type of interface and which method is being invoked.
    Reflect::Interface interface = es::getInterface(proxy->iid);   // XXX Should cache the result

    // If this interface inherits another interface,
    // methodNumber is checked accordingly.
    if (interface.getInheritedMethodCount() + interface.getMethodCount() <= methodNumber)
    {
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
        super = es::getInterface(super.getQualifiedSuperName().c_str());
    }
    Reflect::Method method(Reflect::Method(super.getMethod(methodNumber - baseMethodCount)));

    if (log)
    {
        esReport("upcall[%d:%p]: %s::%s(",
                 interfaceNumber, server, interface.getName().c_str(), method.getName().c_str());
    }

    unsigned long ref;
    bool stringIsInterfaceName = false;
    if (super.getQualifiedSuperName() == "")
    {
        switch (methodNumber - baseMethodCount)
        {
        case 0:  // queryInterface
            stringIsInterfaceName = true;
            break;
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
    record->variant = variant;

    long long result(0);
    int errorCode(0);
    switch (record->getState())
    {
    case UpcallRecord::INIT:
        // Leap into the server process.
        current->leapIntoServer(record);

        // Initialize TLS.
        memmove(reinterpret_cast<void*>(record->ureg.esp),
                server->tlsImage, server->tlsImageSize);
        memset(reinterpret_cast<u8*>(record->ureg.esp) + server->tlsImageSize,
               0, server->tlsSize - server->tlsImageSize);

        record->push(0);                                            // param
        record->push(reinterpret_cast<unsigned>(server->focus));    // start
        record->push(0);                                            // ret
        record->entry(reinterpret_cast<unsigned>(server->startup));

        if (record->label.set() == 0)
        {
            // Make an upcall the server process.
            unsigned x = Core::splHi();
            Core* core = Core::getCurrentCore();
            record->sp0 = core->tss0->sp0;
            core->tss0->sp0 = current->sp0 = record->label.esp;
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

        // Invoke method
        record->ureg.eax = reinterpret_cast<u32>(record->proxy->object);
        record->ureg.edx = record->methodNumber;
        if (log)
        {
            esReport("object: %p, method: %d @ %p:%p\n", record->ureg.eax, record->ureg.edx, record->ureg.eip, record->ureg.esp);
        }
        if (record->label.set() == 0)
        {
            // Make an upcall the server process.
            unsigned x = Core::splHi();
            Core* core = Core::getCurrentCore();
            record->sp0 = core->tss0->sp0;
            core->tss0->sp0 = current->sp0 = record->label.esp;
            Core::splX(x);
            record->ureg.load();
            // NOT REACHED HERE
        }
        else
        {
            const char* iid = Object::iid();

            // Return to the client process.
            Process* client = current->returnToClient();

            // Copy output parameters from the user stack of the server process.
            errorCode = server->copyOut(record, iid, stringIsInterfaceName);

            // Get result code
            if (errorCode == 0)
            {
                result = (static_cast<long long>(record->ureg.edx) << 32) | record->ureg.eax;
                errorCode = record->ureg.ecx;
            }

            if (errorCode == 0)
            {
                void** ip = 0;
                Reflect::Type returnType(record->method.getReturnType());
                switch (returnType.getType())
                {
                case Reflect::kAny:
                    if (record->variant->getType() == Any::TypeObject)
                    {
                        ip = reinterpret_cast<void**>(static_cast<Object*>(*(record->variant)));
                    }
                    result = reinterpret_cast<intptr_t>(record->variant);
                    break;
                case Reflect::kObject:
                    // Convert the received interface pointer to kernel's interface pointer
                    ip = reinterpret_cast<void**>(result);
                    if (ip == 0)
                    {
                        result = 0;
                    }
                    break;
                }

                // Process the returned interface pointer
                if (ip) {
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
                        int n = set(server, reinterpret_cast<Object*>(ip), stringIsInterfaceName ? iid : returnType.getQualifiedName().c_str(), true);
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
        }
        break;
    }

    if (super.getQualifiedSuperName() == "")
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
    core->tss0->sp0 = current->sp0 = record->sp0;
    Core::splX(x);
    memmove(&record->ureg, ureg, sizeof(Ureg));
    record->label.jump();
    // NOT REACHED HERE
}

void Process::
// setFocus(void* (*focus)(void* param)) // [check] focus must be a function pointer.
setFocus(void* focus)
{
    typedef void* (*Focus)(void* param); // [check]
    this->focus = reinterpret_cast<Focus>(focus); // [check]
}

UpcallRecord* Process::
createUpcallRecord(const unsigned stackSize)
{
    // Map a user stack
    void* userStack(static_cast<u8*>(USER_MAX) - ((threadCount + upcallCount + 1) * stackSize));
    userStack = map(userStack, stackSize - Page::SIZE,
                    es::CurrentProcess::PROT_READ | es::CurrentProcess::PROT_WRITE,
                    es::CurrentProcess::MAP_PRIVATE, 0, 0);
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

u8* Process::
copyInString(const char* string, u8* esp)
{
    // Check zero termination
    const char* ptr = string;
    int count = 0;
    do
    {
#if 0
        // TODO: ptr might points to the kernel code/data.
        if (!isValid(ptr, 1))
        {
            return NULL;
        }
#endif
        ++count;
    } while (*ptr++);
    esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
    write(string, count, reinterpret_cast<long long>(esp));
    return esp;
}

Object* Process::
copyInObject(Object* object, const char* iid)
{
    if (!object)
    {
        return object;
    }

    void** ip = reinterpret_cast<void**>(object);
    int n = broker.getInterfaceNo(reinterpret_cast<void*>(ip));
    if (0 <= n)
    {
        UpcallProxy* proxy = &upcallTable[n];
        if (proxy->process == this)
        {
            return static_cast<Object*>(proxy->object);
        }
    }

    // Set up a new system call proxy.
    n = set(syscallTable, object, iid);
    if (0 <= n)
    {
        // Set ip to proxy ip
        ip = &ipt[n];
        object->addRef();
    }
    else
    {
        ip = 0; // XXX should raise an exception
    }
    // Note the reference count to the created syscall proxy must
    // be decremented by one at the end of this upcall.
    return reinterpret_cast<Object*>(ip); // XXX
}

// Note copyIn() is called against the server process, so that
// copyIn() can be called from the kernel thread to make an upcall,
int Process::
copyIn(UpcallRecord* record)
{
    UpcallProxy* proxy(record->proxy);
    int methodNumber(record->methodNumber);
    va_list paramv(record->param);
    int* paramp = reinterpret_cast<int*>(paramv);
    u8* esp(reinterpret_cast<u8*>(record->ureg.esp));
    Ureg* ureg(&record->ureg);

    int argv[sizeof(long long) / sizeof(int) * 9]; // XXX 9
    int argc = 0;
    int* argp = argv;

    int count = 0;

    Reflect::Type returnType = record->method.getReturnType();
    if (returnType.getType() == Reflect::kAny)
    {
        // Make space for the return value of Any type
        count = sizeof(Any);
        esp -= count;
        *argp++ = (int) esp;
    }

    // Set this
    *argp++ = (int) proxy->object;

    switch (returnType.getType())
    {
    case Reflect::kAny:
        // Any op(void* buf, int len);
    case Reflect::kString:
        // const char* op(char* buf, int len, ...);
        count = paramp[1];                          // XXX check count
        esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
        *argp++ = (int) esp;
        *argp++ = count;
        paramp += 2;
        break;
    case Reflect::kSequence: // XXX
        // int op(xxx* buf, int len, ...);
        count = returnType.getSize() * paramp[1];   // XXX check count
        esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
        *argp++ = (int) esp;
        *argp++ = paramp[1];
        paramp += 2;
        break;
    case Reflect::kArray:
        // void op(struct* buf, ...);
        // void op(xxx[x] buf, ...);
        count = returnType.getSize();               // XXX check count
        esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
        *argp++ = (int) esp;
        ++paramp;
        break;
    default:
        break;
    }

    Reflect::Parameter param(record->method.listParameter());
    for (int i = 0; param.next(); ++i)
    {

        Reflect::Type type(param.getType());
        if (log)
        {
            esReport("%s ", param.getName().c_str());
        }

        switch (type.getType())
        {
        case Reflect::kAny:
            {
                Any* var = reinterpret_cast<Any*>(paramp);
                switch (var->getType())
                {
                case Any::TypeString:
                    esp = copyInString(static_cast<const char*>(*var), esp);
                    if (!esp)
                    {
                        return EFAULT;
                    }
                    *var = Any(reinterpret_cast<const char*>(esp));
                    break;
                case Any::TypeObject:
                    *var = Any(reinterpret_cast<Object*>(copyInObject(static_cast<Object*>(*var), Object::iid())));
                    break;
                }
                memcpy(argp, paramp, sizeof(AnyBase));
                argp += sizeof(AnyBase) / sizeof(int);
                paramp += sizeof(AnyBase) / sizeof(int);
                var->makeVariant();
            }
            break;
        case Reflect::kPointer:  // XXX x86 specific
        case Reflect::kBoolean:
        case Reflect::kShort:
        case Reflect::kLong:
        case Reflect::kOctet:
        case Reflect::kUnsignedShort:
        case Reflect::kUnsignedLong:
        case Reflect::kFloat:
            *argp++ = *paramp++;
            break;
        case Reflect::kLongLong:
        case Reflect::kUnsignedLongLong:
        case Reflect::kDouble:
            *argp++ = *paramp++;
            *argp++ = *paramp++;
            break;
        case Reflect::kString:
            esp = copyInString(*reinterpret_cast<char**>(paramp), esp);
            if (!esp)
            {
                return EFAULT;
            }
            *argp++ = (int) esp;
            ++paramp;
            break;
        case Reflect::kSequence:
            // xxx* buf, int len, ...
            count = type.getSize() * paramp[1]; // XXX check count
            esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
            write(*reinterpret_cast<void**>(paramp), count, reinterpret_cast<long long>(esp));
            *argp++ = (int) esp;
            *argp++ = paramp[1];
            paramp += 2;
            break;
        case Reflect::kArray:        // xxx[x] buf, ...
            count = type.getSize();
            esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
            write(*reinterpret_cast<void**>(paramp), count, reinterpret_cast<long long>(esp));
            *argp++ = (int) esp;
            ++paramp;
            break;
        case Reflect::kObject:
            *argp++ = *paramp = (int) copyInObject(*reinterpret_cast<Object**>(paramp), type.getQualifiedName().c_str());  // XXX c_str() can cause a problem here
            // Note the reference count to the created syscall proxy must
            // be decremented by one at the end of this upcall.
            ++paramp;
            break;
        default:
            break;
        }

        if (log && i + 1 < record->method.getParameterCount())
        {
            esReport(", ");
        }
    }
    if (log)
    {
        esReport(");\n");
    }

    // Copy arguments
    esp -= sizeof(int) * (argp - argv);
    write(argv, sizeof(int) * (argp - argv), reinterpret_cast<long>(esp));
    ureg->esp = reinterpret_cast<long>(esp);

    return 0;
}

// Note copyOut() is called against the server process.
// iid is updaed only when stringIsInterfaceName is true
int Process::
copyOut(UpcallRecord* record, const char*& iid, bool stringIsInterfaceName)
{
    UpcallProxy* proxy(record->proxy);
    int methodNumber(record->methodNumber);
    void* paramv(record->param);
    int* paramp = reinterpret_cast<int*>(paramv);
    u8* esp(reinterpret_cast<u8*>(record->ureg.esp));  // XXX should be saved separately

    long long rc = (static_cast<long long>(record->ureg.edx) << 32) | record->ureg.eax;

    if (log)
    {
        Reflect::Interface interface;
        try
        {
            interface = es::getInterface(proxy->iid);
        }
        catch (Exception& error)
        {
            return error.getResult();
        }
        esReport("return from upcall %p: %s::%s(",
                 esp, interface.getName().c_str(), record->method.getName().c_str());
    }

    int count;

    Reflect::Type returnType = record->method.getReturnType();
    if (returnType.getType() == Reflect::kAny)
    {
        count = sizeof(Any);
        esp -= count;
        read(record->variant, count, reinterpret_cast<long long>(esp));
    }
    switch (returnType.getType())
    {
    case Reflect::kAny:
        // Any op(void* buf, int len);
        rc = paramp[1]; // XXX check type and string length if type is string
        if (record->variant->getType() != Any::TypeString)
        {
            break;
        }
        // FALL THROUGH
    case Reflect::kString:
        // const char* op(char* buf, int len, ...);
        count = paramp[1];
        esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
        if (0 < rc)
        {
            // TODO: Check string length
            read(*reinterpret_cast<void**>(paramp), count, reinterpret_cast<long long>(esp));
            record->ureg.eax = reinterpret_cast<u32>(esp);
        }
        paramp += 2;
        break;
    case Reflect::kSequence:
        // int op(xxx* buf, int len, ...);
        count = returnType.getSize() * paramp[1];
        esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
        if (0 < rc && rc <= paramp[1])
        {
            read(*reinterpret_cast<void**>(paramp), returnType.getSize() * rc, reinterpret_cast<long long>(esp));
        }
        paramp += 2;
        break;
    case Reflect::kArray:
        // void op(struct* buf, ...);
        // void op(xxx[x] buf, ...);
        count = returnType.getSize();   // XXX check count
        esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
        read(*reinterpret_cast<void**>(paramp), count, reinterpret_cast<long long>(esp));
        ++paramp;
        break;
    default:
        break;
    }

    int argc = 0;
    Reflect::Parameter param(record->method.listParameter());
    for (int i = 0; param.next(); ++i)
    {
        Reflect::Type type(param.getType());
        if (log)
        {
            esReport("%s", param.getName().c_str());
        }

        void** ip = 0;
        switch (type.getType())
        {
        case Reflect::kAny:
            {
                Any* var = reinterpret_cast<Any*>(paramp);
                if (var->getType() == Any::TypeObject)
                {
                    ip = reinterpret_cast<void**>(static_cast<Object*>(*var));
                }
                paramp += sizeof(AnyBase) / sizeof(int);
            }
            break;
        case Reflect::kPointer:  // XXX x86 specific
        case Reflect::kBoolean:
        case Reflect::kShort:
        case Reflect::kLong:
        case Reflect::kOctet:
        case Reflect::kUnsignedShort:
        case Reflect::kUnsignedLong:
        case Reflect::kFloat:
            ++paramp;
            break;
        case Reflect::kLongLong:
        case Reflect::kUnsignedLongLong:
        case Reflect::kDouble:
            paramp += 2;
            break;
        case Reflect::kString:
            if (stringIsInterfaceName)
            {
                iid = *reinterpret_cast<const char**>(paramp);
            }
            ++paramp;
            break;
        case Reflect::kSequence:
            // xxx* buf, int len, ...
            count = type.getSize() * paramp[1];
            esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
            paramp += 2;
            break;
        case Reflect::kArray:        // xxx[x] buf, ...
            count = type.getSize();
            esp -= (count + sizeof(int) - 1) & ~(sizeof(int) - 1);
            ++paramp;
            break;
        case Reflect::kObject:
            ip = *reinterpret_cast<void***>(paramp);
            ++paramp;
            break;
        default:
            break;
        }

        if (ip && ipt <= ip && ip < ipt + INTERFACE_POINTER_MAX)
        {
            // Release the created syscall proxy.
            SyscallProxy* proxy(&syscallTable[ip - ipt]);
            proxy->release();
        }

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
