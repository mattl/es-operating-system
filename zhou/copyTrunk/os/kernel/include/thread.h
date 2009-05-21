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

#ifndef NINTENDO_ES_KERNEL_THREAD_H_INCLUDED
#define NINTENDO_ES_KERNEL_THREAD_H_INCLUDED

#ifdef __es__

#include <stdarg.h>
#include <stddef.h> // size_t
#include <es.h>
#include <es/any.h>
#include <es/interlocked.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/reflect.h>
#include <es/base/IMonitor.h>
#include <es/base/IProcess.h>
#include <es/base/IRuntime.h>
#include <es/base/IThread.h>
#include "alarm.h"
#include "cpu.h"
#include "label.h"
#include "spinlock.h"

class Core;
class Delegate;
class UpcallProxy;
class Process;
class Sched;
class SpinLock;
class Thread;
class ThreadKey;

class Delegate
{
public:
    virtual int invoke(int result) = 0;
};

template<class C>
class DelegateTemplate : public Delegate
{
    C* instance;
    int (C::*method)(int result);

public:
    DelegateTemplate(C* instance, int (C::*method)(int result)) :
        instance(instance),
        method(method)
    {
    }

    int invoke(int result)
    {
        return (instance->*method)(result);
    }
};

class UpcallRecord
{
    static const int INIT = 0;
    static const int READY = 1;

    int                 state;
    Link<UpcallRecord>  link;
    Link<UpcallRecord>  linkThread;
    Process*            process;    // The server process
    void*               userStack;  // User stack to be used for upcalls
    void*               tcb;        // Thread control block
    Ureg                ureg;       // Where to start upcall in user-land

    Process*            client;     // The client process
    unsigned            sp0;        // Previous kernel stack top address
    Label               label;      // Where to continue executing after upcall

    UpcallProxy*        proxy;
    int                 methodNumber;
    va_list             param;
    Any*                variant;
    Reflect::Method     method;

    friend class Core;
    friend class Thread;
    friend class Process;

    UpcallRecord(Process* process) :
        state(INIT),
        process(process),
        client(0)
    {
        ASSERT(process);
    }

    int getState()
    {
        return state;
    }

    void setState(int state)
    {
        this->state = state;
    }

    void* tls(unsigned size, unsigned align)
    {
        size = (size + align - 1) & ~(align -1);

        ureg.esp -= size;
        tcb = reinterpret_cast<void*>(ureg.esp + size);
        return reinterpret_cast<void*>(ureg.esp);
    }

    void push(unsigned arg)
    {
        unsigned* frame = reinterpret_cast<unsigned*>(ureg.esp);
        *--frame = arg;
        ureg.esp = reinterpret_cast<unsigned>(frame);
    }

    void entry(unsigned long start)
    {
        ASSERT(state == INIT);
        ureg.eip = start;
    }
};

class Thread : public es::Thread, public es::Callback, public Lock
{
    friend class Sched;
    friend class Process;
    friend int esInit(Object** nameSpace);

public:
    static Sched*       sched;      // XXX

    Ref                 ref;
    Link<Thread>        link;
    int                 priority;   // the higher the integer, the higher the priority.

    int                 state;
    unsigned            attr;
    Core*               core;

    int                 base;       // base priority

    unsigned            error;

    void**              rval;
    const void*         val;

    void*               stack;
    unsigned            stackSize;
    unsigned            sp0;
    void*               ktcb;
    Label               label;
#ifdef __i386__
    Xreg                xreg;
#endif

    class Queue : public List<Thread, &Thread::link>
    {
    public:
        void addPrio(Thread* thread)
        {
            Thread* t;
            Queue::Iterator iter = begin();
            while ((t = iter.next()))
            {
                if (t->priority < thread->priority)
                {
                    iter.previous();
                    break;
                }
            }
            iter.add(thread);
        };
    };

    class Rendezvous : public Lock
    {
        Queue queue;

    public:
        void sleep(Delegate* delegate);
        void wakeup(Delegate* delegate = 0);
        int getPriority();

        void remove(Thread* thread);
        void update(Thread* thread, int effective);
    };

    class Monitor : public es::Monitor, public es::Callback
    {
        friend class Thread;

        Ref             ref;
        Rendezvous      rendezvous;
        int             lockCount;
        Rendezvous      cv;
        Thread*         owner;      // the current owner
        Link<Monitor>   link;

        int condLock(int);
        int condTryLock(int);
        int condUnlock(int);
        int condWait(int);

        int invoke(int);

    public:
        Monitor();

        int getPriority();
        void update();

        void spinLock();
        void spinUnlock();

        // IInterface
        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();

        void lock();
        bool tryLock();
        void unlock();

        // IMonitor
        bool wait();
        bool wait(s64 timeout);
        void notify();
        void notifyAll();

        class Synchronized
        {
            Monitor&    monitor;

            Synchronized& operator=(const Synchronized&);

        public:
            Synchronized(Monitor& monitor) : monitor(monitor)
            {
                monitor.lock();
            }
            ~Synchronized()
            {
                monitor.unlock();
            }
        };

        // [Constructor]
        class Constructor : public es::Monitor::Constructor
        {
        public:
            es::Monitor* createInstance();
            Object* queryInterface(const char* riid);
            unsigned int addRef();
            unsigned int release();
        };

        static void initializeConstructor();
    };

    typedef List<Monitor, &Monitor::link> MonitorList;

    Rendezvous*         rendezvous; // rendezvous currently sleeping
    Monitor*            monitor;    // monitor trying to lock

    Rendezvous          joinPoint;
    MonitorList         monitorList;

    Alarm               alarm;
    Rendezvous          sleepPoint;

    void* (*func)(void*);
    void* param;

    // Process related
    Process*            process;
    void*               userStack;
    Link<Thread>        linkProcess;
    void*               tcb;
    List<UpcallRecord, &UpcallRecord::linkThread>
                        upcallList;     // List of upcall records

    Thread(void* (*run)(void*), void* param, int priority,
           void* stack = 0, unsigned stackSize = 32768);

    ~Thread();

    void setState(int state)
    {
        this->state = state;
    }

    void setRun();
    void unsetRun();

    bool isAlive();

    bool holdsLock(Monitor* monitor);

    int condJoin(int);
    int condSleep(int);

    int getEffectivePriority();
    void updatePriority();
    Thread* setEffectivePriority(int priority);
    Thread* resetPriority();

    bool isDeadlocked();
    void unlockAllMonitors();

    /** Push a parameter to the user stack.
     */
    void push(unsigned param);

    /** Set the entry point in the user address space to start.
     */
    void entry(unsigned long start);

    /** Reserve the tls in the user stack.
     */
    void* tls(unsigned size, unsigned align = sizeof(int));

    /** Push argc and argv to the user stack.
     */
    void setArguments(char* arguments);

    Process* returnToClient();
    Process* leapIntoServer(UpcallRecord* record);

    bool checkStack();

    //
    // ICurrentThread (called by Sched)
    //
    void exit(void* val);
    void sleep(long long timeout);
    int setCancelState(int state);
    int setCancelType(int type);
    void testCancel();
    int getState()
    {
        return state;
    }

    //
    // ICallback
    //
    int invoke(int result);

    //
    // IThread
    //
    void start();
    int getPriority();
    void setPriority(int priority);
    void* join();
    void cancel();

    //
    // IInterface
    //
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    static void startUp(void* param);

    // Architecture specific functions
    static Thread* getCurrentThread();
    static void reschedule();
};

class Sched : public es::CurrentThread, public es::CurrentProcess, public es::Runtime,
              public es::Callback, public Lock
{
    friend class Core;
    friend class Lock;
    friend class SpinLock;
    friend class Thread;

    Ref                 ref;
    volatile unsigned   runQueueBits;
    bool                runQueueHint;
    Thread::Queue       runQueue[es::Thread::Highest + 1];

    static Ref          numCores;

public:
    Sched();
    void setRun(Thread* thread);
    void unsetRun(Thread* thread);
    Thread* selectThread();

    // ICallback
    int invoke(int result);

    //
    // ICurrentThread
    //
    void exit(void* val);
    void sleep(long long timeout);
    int setCancelState(int state);
    int setCancelType(int type);
    void testCancel();
    int getState();

    // ICurrentProcess
    void exit(int status);
    void* map(void* start, long long length, unsigned int prot, unsigned int flags,
              es::Pageable* pageable, long long offset);
    void unmap(void* start, long long length);
    es::CurrentThread* currentThread();
    // [check] start must be a function pointer.
    // es::Thread* createThread(void* (*start)(void* param), void* param);
    es::Thread* createThread(void* start, void* param);
    void yield();
    es::Monitor* createMonitor();
    es::Context* getRoot();
    es::Stream* getInput();
    es::Stream* getOutput();
    es::Stream* getError();
    void* setBreak(long long increment);
    long long getNow();
    bool trace(bool on);
    void setCurrent(es::Context* context);
    es::Context* getCurrent();

    // IRuntime
    /* [check] function pointer.
    void setStartup(void (*startup)(void* (*start)(void* param), void* param));
    void setFocus(void* (*focus)(void* param));
    */
    void setStartup(void* startup);
    void setFocus(void* focus);

    //
    // IInterface
    //
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

typedef Thread::Monitor     Monitor;
typedef Thread::MonitorList MonitorList;
typedef Thread::Rendezvous  Rendezvous;

#else   // !__es__

#include "core.h"

#endif  // !__es__

#endif  // NINTENDO_ES_KERNEL_THREAD_H_INCLUDED
