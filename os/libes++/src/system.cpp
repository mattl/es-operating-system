/*
 * Copyright 2011 Esrille Inc.
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
    void registerConstructor(const char* iid, Object* constructor);
}  // namespace es

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

class System : public es::CurrentProcess
{
    struct CurrentThread : public es::CurrentThread
    {
        es::CurrentThread* currentThread;

        CurrentThread(es::CurrentThread* currentThread) :
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

        Object* queryInterface(const char* riid)
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

    es::CurrentProcess* currentProcess;
    CurrentThread    current;

public:
    System() :
        currentProcess(reinterpret_cast<es::CurrentProcess*>(&(broker.getInterfaceTable()[0]))),
        current(currentProcess->currentThread())
    {
        es::Runtime* runtime;
        runtime = reinterpret_cast<es::Runtime*>(currentProcess->queryInterface(es::Runtime::iid()));
        if (runtime)
        {
            runtime->setStartup(reinterpret_cast<void*>(start)); // [check] cast.
            runtime->setFocus(reinterpret_cast<void*>(focus)); // [check] cast.
            runtime->release();
        }

        // Update constructors.
        if (Handle<es::Context> root = currentProcess->getRoot())
        {
            if (Handle<es::Iterator> iterator = root->list("class"))
            {
                while (iterator->hasNext())
                {
                    if (Handle<es::Binding> binding = iterator->next())
                    {
                        char iid[1024];
                        char ciid[1024];
                        binding->getName(iid, sizeof iid);
                        strcpy(ciid, iid);
                        strcat(ciid, "::Constructor");
                        if (Object* unknown = binding->getObject())
                        {
                            void* constructor = unknown->queryInterface(ciid);
                            es::registerConstructor(iid, reinterpret_cast<Object*>(constructor));
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

    void* map(void* start, long long length, unsigned int prot, unsigned int flags, es::Pageable* pageable, long long offset)
    {
        return currentProcess->map(start, length, prot, flags, pageable, offset);
    }

    void unmap(void* start, long long length)
    {
        currentProcess->unmap(start, length);
    }

    es::CurrentThread* currentThread()
    {
        current.addRef();
        return &current;
    }

    // es::Thread* createThread(void* (*start)(void* param), void* param) // [check] function pointer.
    es::Thread* createThread(void* start, void* param)
    {
        return currentProcess->createThread(start, param);
    }

    void yield()
    {
        currentProcess->yield();
    }

    es::Monitor* createMonitor()
    {
        return currentProcess->createMonitor();
    }

    es::Context* getRoot()
    {
        return currentProcess->getRoot();
    }

    es::Stream* getInput()
    {
        return currentProcess->getInput();
    }

    es::Stream* getOutput()
    {
        return currentProcess->getOutput();
    }

    es::Stream* getError()
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

    void setCurrent(es::Context* context)
    {
        return currentProcess->setCurrent(context);
    }

    es::Context* getCurrent()
    {
        return currentProcess->getCurrent();
    }

    Object* queryInterface(const char* riid)
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

es::CurrentProcess* System() __attribute__((weak));

es::CurrentProcess* System()
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
    __asm__ __volatile__ (
        "leave\n"
        "ret\n"
    );
    // Note we cannot assume that the function epilogue generated by the
    // compiler uses the ordinary leave and ret pair since the compiler
    // cannot tell extra arguments has been pushed inside this function.
    // For instance, gcc 4.6.1 chooses the add, pop, and ret sequence.
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
