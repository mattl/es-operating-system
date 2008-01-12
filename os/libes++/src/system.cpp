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
            rval = val;
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

        bool queryInterface(const Guid& riid, void** objectPtr)
        {
            return currentThread->queryInterface(riid, objectPtr);
        }

        unsigned int addRef(void)
        {
            return currentThread->addRef();
        }

        unsigned int release(void)
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
        currentProcess->queryInterface(IID_IRuntime, (void**) &runtime);
        if (runtime)
        {
            runtime->setStartup(start);
            runtime->setFocus(focus);
            runtime->release();
        }
    }

    void exit(int status)
    {
        currentProcess->exit(status);
    }

    void* map(const void* start, long long length, unsigned int prot, unsigned int flags, IPageable* pageable, long long offset)
    {
        return currentProcess->map(start, length, prot, flags, pageable, offset);
    }

    void unmap(const void* start, long long length)
    {
        currentProcess->unmap(start, length);
    }

    ICurrentThread* currentThread()
    {
        current.addRef();
        return &current;
    }

    IThread* createThread(void* (*start)(void* param), void* param)
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

    IStream* getIn()
    {
        return currentProcess->getIn();
    }

    IStream* getOut()
    {
        return currentProcess->getOut();
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

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        return currentProcess->queryInterface(riid, objectPtr);
    }

    unsigned int addRef(void)
    {
        return currentProcess->addRef();
    }

    unsigned int release(void)
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

static System current __attribute__ ((init_priority (102)));

ICurrentProcess* System() __attribute__((weak));

ICurrentProcess* System()
{
    return &current;
}

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

int esReport(const char* spec, ...) __attribute__((weak));
int esReportv(const char* spec, va_list list) __attribute__((weak));
void esPanic(const char* file, int line, const char* msg, ...) __attribute__((weak));

int esReport(const char* spec, ...)
{
    va_list list;
    int count;

    va_start(list, spec);
    count = esReportv(spec, list);
    va_end(list);
    return count;
}

int esReportv(const char* spec, va_list list)
{
    IStream* output(System()->getOut());
    Formatter textOutput(output);
    int count = textOutput.format(spec, list);
    output->release();
    return count;
}

void esPanic(const char* file, int line, const char* msg, ...)
{
    va_list marker;

    va_start(marker, msg);
    esReportv(msg, marker);
    va_end(marker);
    esReport(" in \"%s\" on line %d.\n", file, line);

    System()->exit(1);
}

DateTime DateTime::getNow()
{
    return DateTime(System()->getNow());
}
