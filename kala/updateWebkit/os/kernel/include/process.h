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

#ifndef NINTENDO_ES_KERNEL_PROCESS_H_INCLUDED
#define NINTENDO_ES_KERNEL_PROCESS_H_INCLUDED

#ifdef __es__

#include <errno.h>
#include <es/broker.h>
#include <es/exception.h>
#include <es/ref.h>
#include <es/list.h>
#include <es/base/IProcess.h>
#include <es/base/IRuntime.h>
#include "cache.h"
#include "thread.h"
#include "zero.h"

class InterfaceDescriptor;
class Map;
class Process;

class SyscallProxy : public Object
{
    Ref         ref;
    Interlocked use;
    void*       object;
    const char* iid;

public:
    SyscallProxy() :
        ref(0),
        object(0),
        iid(0)
    {
    }

    bool set(void* object, const char* iid, bool used = false);

    void* getObject() const
    {
        return object;
    }

    const char* getIID() const
    {
        return iid;
    }

    bool isValid() const
    {
        return (0 < ref && 0 <= use) ? true : false;
    }

    long addUser();
    long releaseUser();

    // IInterface
    Object* queryInterface(const char* riid)
    {
        ASSERT(false);
        return NULL;
    }
    unsigned int addRef();
    unsigned int release();

    friend class Process;
};

class UpcallProxy : public Object
{
    Ref         ref;
    Interlocked use;
    void*       object;
    const char* iid;
    Process*    process;

public:
    UpcallProxy() :
        ref(0),
        object(0),
        iid(0),
        process(0)
    {
    }

    bool set(Process* process, void* object, const char* iid, bool used);

    bool isUsed();

    // IInterface
    Object* queryInterface(const char* riid)
    {
        ASSERT(false);
        return NULL;
    }
    unsigned int addRef();
    unsigned int release();

    friend class Process;
};

class Mmu
{
    static u32*     kernelDirectory;

    Cache*          pageTable;
    u32*            directory;

public:
    Mmu(Cache* pageTable);
    ~Mmu();

    /** Get the PTE value at the specified address.
     */
    unsigned long get(const void* addr);

    void set(const void* addr, Page* page, unsigned long pte);
    void unset(const void* addr, long count, es::Pageable* pageable, long long offset);

    void load();
    void invalidate(const void* addr);

    static void* getPointer(unsigned long pte);
};

class Swap
{
    static const int NUM_ENTRIES = 1024 - 6;    // XXX

    struct SwapUse
    {
        unsigned long   offset;
        u16             freeCount;
        u16             freeIndex;
        Link<SwapUse>   linkFree;
        Link<SwapUse>   linkFull;
        u16             map[NUM_ENTRIES];
        u16             ref[NUM_ENTRIES];

        SwapUse(unsigned long offset);

        typedef List<SwapUse, &SwapUse::linkFree>   FreeList;
        typedef List<SwapUse, &SwapUse::linkFull>   FullList;
    };

    Cache*              cache;
    SwapUse::FreeList   freeList;
    SwapUse::FullList   fullList;
    long                size;

    SwapUse* getSwapUse(unsigned long offset);

public:
    Swap(Cache* cache);
    ~Swap();
    void put(unsigned long offset);
    void put(Page* page);
    Page* restore(unsigned long offset);
    Page* get();
};

class Map
{
    friend class Process;

    Link<Map>   link;

    Process*    proc;
    const void* start;
    const void* end;
    long        length;
    es::Pageable*  pageable;
    long long   offset;
    unsigned    prot;
    unsigned    flags;

    typedef ::List<Map, &Map::link>   List;

    Map(Process* proc, const void* start, const void* end, long length,
        unsigned prot, unsigned flags,
        es::Pageable* pagealbe, long long offset);
    ~Map();

    long getOffset(const void* addr)
    {
        return static_cast<const u8*>(addr) - static_cast<const u8*>(start);
    }

    /** Gets the position in the pageable object that corresponds to this address
     */
    long long getPosition(const void* addr);
};

class Process : public es::Process, public es::Runtime
{
public:
    static void* const USER_MIN;
    static void* const USER_MAX;

    static const unsigned INTERFACE_POINTER_MAX = 100;

private:
    static Zero*    zero;
    static Swap*    swap;

    Ref             ref;
    Monitor         monitor;

    Map::List       mapList;
    Mmu*            mmu;
    void*           end;    // break value

    void**          ipt;    // XXX rename this field
    SyscallProxy    syscallTable[INTERFACE_POINTER_MAX];

    int             exitValue;
    Rendezvous      waitPoint;

    void            (*startup)(void* (*start)(void* param), void* param);
    void*           tlsImage;
    unsigned        tlsImageSize;
    unsigned        tlsSize;
    unsigned        tlsAlign;

    int             threadCount;
    List<Thread, &Thread::linkProcess>
                    threadList;

    es::Context*       root;
    es::Context*       current;
    es::Stream*        in;
    es::Stream*        out;
    es::Stream*        error;

    bool            log;

    // Upcall related fields:
    Lock            spinLock;
    void*           (*focus)(void* param);
    Interlocked     upcallCount;    // Number of UpcallRecords allocated to this process
    List<UpcallRecord, &UpcallRecord::link>
                    upcallList;     // List of free upcall records
    List<UpcallRecord, &UpcallRecord::link>
                    activatedList;  // List of upcall records in use

    int condWait(int);

public:
    Process();
    ~Process();

    void load();

    Map* lookup(const void* addr);
    bool isValid(const void* start, long long length)
    {
        const void* end(static_cast<const u8*>(start) + length);
        if (start < USER_MIN || USER_MAX < end || end < start)
        {
            return false;
        }
        return true;
    }
    bool isValid(const void* start, long long length, bool write);
    void dump();

    int validityFault(const void* addr, u32 error);
    int protectionFault(const void* addr, u32 error);

    int read(void* dst, int count, long long offset);
    int write(const void* src, int count, long long offset);

    long long systemCall(void** self, unsigned methodNumber, va_list param, void** base);
    int set(SyscallProxy* table, void* object, const char* iid, bool used = false);

    Thread* createThread(const unsigned stackSize);
    void detach(Thread* thread);

    void* map(void* start, long long length, unsigned int prot, unsigned int flags,
              es::Pageable* pageable, long long offset, long long size,
              void* min, void* max);

    // ICurrentProcess related (called by Sched)
    void exit(int status);
    void* map(void* start, long long length, unsigned int prot, unsigned int flags,
              es::Pageable* pageable, long long offset);
    void unmap(void* start, long long length);
    es::Thread* createThread(void* (*start)(void* param), void* param);
    es::Context* getRoot();
    es::Stream* getInput();
    es::Stream* getOutput();
    es::Stream* getError();
    void* setBreak(long long increment);
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

    // IProcess
    void kill();
    void start();
    void start(es::File* file);
    void start(es::File* file, const char* arguments);
    int wait();
    int getExitValue();
    bool hasExited();
    void setRoot(es::Context* root);
    void setInput(es::Stream* in);
    void setOutput(es::Stream* out);
    void setError(es::Stream* error);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    // Initializes the static members
    static void initialize();
    static void* ast(void* param);
    static Process* getCurrentProcess();

    friend class Core;
    friend class Mmu;
    friend class Swap;

    static long long upcall(void* self, void* base, int m, va_list ap);
    static Broker<upcall, INTERFACE_POINTER_MAX> broker;
    static UpcallProxy upcallTable[INTERFACE_POINTER_MAX];
    static int set(Process* process, void* object, const char* iid, bool used);

    UpcallRecord* createUpcallRecord(const unsigned stackSize);
    UpcallRecord* getUpcallRecord();
    void putUpcallRecord(UpcallRecord* record);
    void returnFromUpcall(Ureg* ureg);

    /** Copies the parameters to the user stack of the server process.
     * The space for the input parameters must also be reserved in the
     * server user stack so that they can be copied back later.
     * @return  error number
     */
    int copyIn(UpcallRecord* record);
    u8* copyInString(const char* string, u8* esp);
    Object* copyInObject(Object* object, const char* iid);

    /** Copies back the input parameters from the server user stack.
     * @return  error number
     */
    int copyOut(UpcallRecord* record, const char*& iid, bool stringIsInterfaceName);

    // [Constructor]
    class Constructor : public es::Process::Constructor
    {
    public:
        es::Process* createInstance();
        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();
    };

    static void initializeConstructor();
};

#endif // __es__

#endif // NINTENDO_ES_KERNEL_PROCESS_H_INCLUDED
