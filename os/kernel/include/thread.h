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

#ifndef NINTENDO_ES_KERNEL_THREAD_H_INCLUDED
#define NINTENDO_ES_KERNEL_THREAD_H_INCLUDED

#ifdef __es__

#include <stdarg.h>
#include <stddef.h> // size_t
#include <es.h>
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
    Reflect::Function   method;

    friend class Core;
    friend class Thread;
    friend class Process;

    UpcallRecord(Process* process) :
        state(INIT),
        process(process),
        client(0)
    {
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

class Thread : public IThread, public ICallback, public Lock
{
    friend class Sched;
    friend class Process;
    friend int esInit(IInterface** nameSpace);

public:
    static Sched*       sched;      // XXX

    Ref                 ref;
    Link<Thread>        link;
    int                 priority;   // the higher the integer, the higher the priority.

    State               state;
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

    class Monitor : public IMonitor, public ICallback
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
        bool queryInterface(const Guid& riid, void** objectPtr);
        unsigned int addRef(void);
        unsigned int release(void);

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

    void setState(State state)
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
    bool join(void** rval);
    void cancel();

    //
    // IInterface
    //
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);

    static void startUp(void* param);

    // Architecture specific functions
    static Thread* getCurrentThread();
    static void reschedule();
};

class Sched : public ICurrentThread, public ICurrentProcess, public IRuntime,
              public ICallback, public Lock
{
    friend class Core;
    friend class Lock;
    friend class SpinLock;
    friend class Thread;

    Ref                 ref;
    volatile unsigned   runQueueBits;
    bool                runQueueHint;
    Thread::Queue       runQueue[IThread::Highest + 1];

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
    void* map(const void* start, long long length, unsigned int prot, unsigned int flags,
              IPageable* pageable, long long offset);
    void unmap(const void* start, long long length);
    ICurrentThread* currentThread();
    IThread* createThread(void* (*start)(void* param), void* param);
    void yield(void);
    IMonitor* createMonitor();
    IContext* getRoot();
    IStream* getIn();
    IStream* getOut();
    IStream* getError();
    void* setBreak(long long increment);
    long long getNow();
    bool trace(bool on);

    // IRuntime
    void setStartup(void (*startup)(void* (*start)(void* param), void* param));
    void setFocus(void* (*focus)(void* param));

    //
    // IInterface
    //
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);
};

typedef Thread::Monitor     Monitor;
typedef Thread::MonitorList MonitorList;
typedef Thread::Rendezvous  Rendezvous;

#endif  // __es__

#ifdef __unix__
#include "core.h"
#endif  // __unix__

#endif  // NINTENDO_ES_KERNEL_THREAD_H_INCLUDED
