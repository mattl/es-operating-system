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
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unwind.h>
#include <es.h>
#include <es/broker.h>
#include <es/dateTime.h>
#include <es/exception.h>
#include <es/formatter.h>
#include <es/handle.h>
#include <es/ref.h>
#include <es/base/IProcess.h>
#include <es/base/IRuntime.h>

namespace es
{
    void registerConstructor(const char* iid, IInterface* constructor);
}  // namespace es

using namespace es;

static __thread void* stopCfa;
static __thread jmp_buf stopBuf;
static __thread struct _Unwind_Exception exc;
static __thread void* rval;

long long _syscall(void* self, void* base, int m, va_list ap)
{
    long long result;
    int error(0);

    __asm__ __volatile__ (
        "int    $65"
        : "=A"(result), "=c" (error) : "a"(self), "d"(m), "c"(ap), "S"(base));
    if (error)
    {
        esThrow(error);
    }
    return result;
}

static Broker<_syscall, 100> broker __attribute__ ((init_priority (101)));

class System : public ICurrentProcess
{
    struct CurrentThread : public ICurrentThread
    {
        ICurrentThread* currentThread;

        CurrentThread(ICurrentThread* currentThread) :
            currentThread(currentThread)
        {
        }

        ~CurrentThread()
        {
            release();
        }

        void exit(void* val)
        {
            // Call destructors before terminating the current thread.
            rval = const_cast<void*>(val);
            exc.exception_class = 0;
            exc.exception_cleanup = System::cleanup;
            _Unwind_ForcedUnwind(&exc, System::stop, stopBuf);
        }

        void sleep(long long timeout)
        {
            return currentThread->sleep(timeout);
        }

        int setCancelState(int state)
        {
            return currentThread->setCancelState(state);
        }

        int setCancelType(int type)
        {
            return currentThread->setCancelType(type);
        }

        void testCancel()
        {
            currentThread->testCancel();
        }

        void* queryInterface(const char* riid)
        {
            return currentThread->queryInterface(riid);
        }

        unsigned int addRef()
        {
            return currentThread->addRef();
        }

        unsigned int release()
        {
            return currentThread->release();
        }
    };

    ICurrentProcess* currentProcess;
    CurrentThread    current;

public:
    System() :
        currentProcess(reinterpret_cast<ICurrentProcess*>(&(broker.getInterfaceTable()[0]))),
        current(currentProcess->currentThread())
    {
        IRuntime* runtime;
        runtime = reinterpret_cast<IRuntime*>(currentProcess->queryInterface(IRuntime::iid()));
        if (runtime)
        {
            runtime->setStartup(reinterpret_cast<void*>(start)); // [check] cast.
            runtime->setFocus(reinterpret_cast<void*>(focus)); // [check] cast.
            runtime->release();
        }

        // Update constructors.
        if (Handle<IContext> root = currentProcess->getRoot())
        {
            if (Handle<IIterator> iterator = root->list("class"))
            {
                while (iterator->hasNext())
                {
                    if (Handle<IBinding> binding = iterator->next())
                    {
                        char iid[1024];
                        char ciid[1024];
                        binding->getName(iid, sizeof iid);
                        strcpy(ciid, iid);
                        strcat(ciid, "::Constructor");
                        if (IInterface* unknown = binding->getObject())
                        {
                            void* constructor = unknown->queryInterface(ciid);
                            registerConstructor(iid, reinterpret_cast<IInterface*>(constructor));
                            unknown->release();
                        }
                    }
                }
            }
        }
    }

    void exit(int status)
    {
        currentProcess->exit(status);
    }

    void* map(void* start, long long length, unsigned int prot, unsigned int flags, IPageable* pageable, long long offset)
    {
        return currentProcess->map(start, length, prot, flags, pageable, offset);
    }

    void unmap(void* start, long long length)
    {
        currentProcess->unmap(start, length);
    }

    ICurrentThread* currentThread()
    {
        current.addRef();
        return &current;
    }

    // IThread* createThread(void* (*start)(void* param), void* param) // [check] function pointer.
    IThread* createThread(void* start, void* param)
    {
        return currentProcess->createThread(start, param);
    }

    void yield()
    {
        currentProcess->yield();
    }

    IMonitor* createMonitor()
    {
        return currentProcess->createMonitor();
    }

    IContext* getRoot()
    {
        return currentProcess->getRoot();
    }

    IStream* getInput()
    {
        return currentProcess->getInput();
    }

    IStream* getOutput()
    {
        return currentProcess->getOutput();
    }

    IStream* getError()
    {
        return currentProcess->getError();
    }

    void* setBreak(long long increment)
    {
        return currentProcess->setBreak(increment);
    }

    long long getNow()
    {
        return currentProcess->getNow();
    }

    bool trace(bool on)
    {
        return currentProcess->trace(on);
    }

    void setCurrent(IContext* context)
    {
        return currentProcess->setCurrent(context);
    }

    IContext* getCurrent()
    {
        return currentProcess->getCurrent();
    }

    void* queryInterface(const char* riid)
    {
        return currentProcess->queryInterface(riid);
    }

    unsigned int addRef()
    {
        return currentProcess->addRef();
    }

    unsigned int release()
    {
        return currentProcess->release();
    }

    static void start(void* (*func)(void*), void* param);

    static void* focus(void* param);

    static _Unwind_Reason_Code stop(int version,
                                    _Unwind_Action actions,
                                    _Unwind_Exception_Class excClass,
                                    struct _Unwind_Exception* exc,
                                    struct _Unwind_Context* context,
                                    void* stopParameter);

    static void cleanup(_Unwind_Reason_Code reason, struct _Unwind_Exception* exc);
};

static System current __attribute__((init_priority(1001)));    // After InterfaceStore

ICurrentProcess* System() __attribute__((weak));

ICurrentProcess* System()
{
    return &current;
}

extern "C" void esDeallocateSpecific(void);

void System::
start(void* (*func)(void*), void* param)
{
    stopCfa = &func;
    if (setjmp(stopBuf) == 0)
    {
        rval = func(param);
    }
    esDeallocateSpecific(); // Deallocate TSD.
    ::current.current.currentThread->exit(rval);
}

_Unwind_Reason_Code System::
stop(int version,
     _Unwind_Action actions,
     _Unwind_Exception_Class excClass,
     struct _Unwind_Exception* exc,
     struct _Unwind_Context* context,
     void* stopParameter)
{
    if ((actions & _UA_END_OF_STACK) || stopCfa <= (void*) _Unwind_GetCFA(context))
    {
        longjmp(stopBuf, 1);
    }
    return _URC_NO_REASON;
}

void System::
cleanup(_Unwind_Reason_Code reason, struct _Unwind_Exception* exc)
{
    abort();
}

long long esHall()
{
    typedef long long (*Method)(...);
    Method** object;
    int      method;

    __asm__ __volatile__ (
        "int    $66\n"
        : "=a"(object), "=d"(method));
    (*object)[method]();
}

void* System::
focus(void* param)
{
    int errorCode(0);

    try
    {
        for (;;)
        {
            esHall();
        }
    }
    catch (Exception& error)
    {
        errorCode = error.getResult();
    }
    catch (std::bad_alloc)
    {
        errorCode = ENOMEM;
    }
    catch (...)
    {
        // Unexpected exception
        errorCode = EINVAL;
    }
    return reinterpret_cast<void*>(errorCode);
}
