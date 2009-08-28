/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_KERNEL_POSIX_CORE_H_INCLUDED
#define NINTENDO_ES_KERNEL_POSIX_CORE_H_INCLUDED

#include <pthread.h>
#include <es.h>
#include <es/ref.h>
#include <es/base/IMonitor.h>
#include <es/base/IProcess.h>
#include <es/base/IThread.h>

class Core;
class Thread;
class Monitor;
class SpinLock;

class Core : public es::CurrentThread, public es::CurrentProcess
{
    Ref ref;

public:
    // es::CurrentProcess
    void exit(int status);
    void* map(void* start, long long length, unsigned int prot, unsigned int flags, es::Pageable* pageable, long long offset);
    void unmap(void* start, long long length);
    es::CurrentThread* currentThread();
    es::Thread* createThread(void* (*start)(void* param), void* param);
    void yield();
    es::Monitor* createMonitor();
    es::Context* getRoot();
    es::Stream* getInput();
    es::Stream* getOutput();
    es::Stream* getError();
    void* setBreak(long long increment);
    long long getNow();
    void setStartup(void (*startup)(void* (*start)(void* param), void* param));

    // es::CurrentThread
    void exit(const void* val);
    void sleep(long long timeout);
    int setCancelState(int state);
    int setCancelType(int type);
    void testCancel();

    // Object
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

class Monitor : public es::Monitor
{
    Ref             ref;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;

public:
    Monitor();
    ~Monitor();

    // es::Monitor
    void lock();
    bool tryLock();
    void unlock();
    bool wait();
    bool wait(s64 timeout);
    void notify();
    void notifyAll();

    // Object
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    // Internal
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

class Thread : public es::Thread
{
    static pthread_key_t cleanupKey;
    static const int MaxSpecific = 32;
    static void (*dtorTable[MaxSpecific])(void*);

    Ref             ref;
    int             state;
    int             priority;
    void*         (*run)(void*);
    void*           param;
    pthread_t       thread;
    const void*     errorCode;
    const void*     specific[MaxSpecific];

    void exit(const void* errorCode);
    void* getSpecific(int index);
    void setSpecific(int index, const void* ptr);

    static void* init(void* arg);
    static void cleanup(void* arg);

public:
    Thread(void* (*run)(void*), void* param, int priority,
        void* stack = 0, unsigned stackSize = 0);
    ~Thread();

    // es::Thread
    int getState();
    void start();
    int getPriority();
    void setPriority(int priority);
    void* join();
    void cancel();

    // Object
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    static Thread* getCurrentThread();
    static void reschedule();

    friend void Core::exit(const void* val);

    friend void Monitor::lock();
    friend bool Monitor::wait();
    friend bool Monitor::wait(s64 timeout);

    friend void esInitThread();
    friend void esSleep(s64 timeout);
};

class Lock : public Monitor
{
    using Monitor::wait;
    using Monitor::notify;
    using Monitor::notifyAll;
    using Monitor::queryInterface;
    using Monitor::addRef;
    using Monitor::release;
};

class SpinLock : public Monitor
{
    using Monitor::wait;
    using Monitor::notify;
    using Monitor::notifyAll;
    using Monitor::queryInterface;
    using Monitor::addRef;
    using Monitor::release;
};

#include "cache.h"

int esInit(Object** nameSpace);
es::Thread* esCreateThread(void* (*start)(void* param), void* param);
es::Monitor* esCreateMonitor();

#endif // NINTENDO_ES_KERNEL_POSIX_CORE_H_INCLUDED
