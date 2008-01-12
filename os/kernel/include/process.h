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

#ifndef NINTENDO_ES_KERNEL_PROCESS_H_INCLUDED
#define NINTENDO_ES_KERNEL_PROCESS_H_INCLUDED

#ifdef __es__

#include <es/broker.h>
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

class SyscallProxy : public IInterface
{
    Ref         ref;
    Interlocked use;
    void*       object;
    Guid        iid;

public:
    SyscallProxy() :
        ref(0),
        object(0),
        iid(GUID_NULL)
    {
    }

    bool set(void* object, const Guid& iid);

    void* getObject() const
    {
        return object;
    }

    const Guid& getIID() const
    {
        return iid;
    }

    bool isValid() const
    {
        return (0 < ref && 0 < use) ? true : false;
    }

    long addUser();
    long releaseUser();

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        ASSERT(false);
        return false;
    }
    unsigned int addRef();
    unsigned int release();

    friend class Process;
};

class UpcallProxy : public IInterface
{
    Ref         ref;
    Interlocked use;
    void*       object;
    Guid        iid;
    Process*    process;

public:
    UpcallProxy() :
        ref(0),
        object(0),
        iid(GUID_NULL),
        process(0)
    {
    }

    bool set(Process* process, void* object, const Guid& iid);
    bool isUsed();

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        ASSERT(false);
        return false;
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
    void unset(const void* addr, long count, IPageable* pageable, long long offset);

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
    IPageable*  pageable;
    long long   offset;
    unsigned    prot;
    unsigned    flags;

    typedef List<Map, &Map::link>   List;

    Map(Process* proc, const void* start, const void* end, long length,
        unsigned prot, unsigned flags,
        IPageable* pagealbe, long long offset);
    ~Map();

    long getOffset(const void* addr)
    {
        return static_cast<const u8*>(addr) - static_cast<const u8*>(start);
    }

    /** Gets the position in the pageable object that corresponds to this address
     */
    long long getPosition(const void* addr);
};

class Process : public IProcess, public IRuntime
{
public:
    static void* const USER_MIN;
    static void* const USER_MAX;

    static const unsigned INTERFACE_POINTER_MAX = 100;

private:
    static CacheFactory*    cacheFactory;
    static Zero*            zero;
    static Swap*            swap;

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

    IContext*       root;
    IStream*        in;
    IStream*        out;
    IStream*        error;

    bool            log;

    // Upcall related fields:
    SpinLock        spinLock;
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
    int set(SyscallProxy* table, void* object, const Guid& iid);

    Thread* createThread(const unsigned stackSize);
    void detach(Thread* thread);

    void* map(const void* start, long long length, unsigned int prot, unsigned int flags,
              IPageable* pageable, long long offset, long long size,
              void* min, void* max);

    // ICurrentProcess related (called by Sched)
    void exit(int status);
    void* map(const void* start, long long length, unsigned int prot, unsigned int flags,
              IPageable* pageable, long long offset);
    void unmap(const void* start, long long length);
    IThread* createThread(void* (*start)(void* param), void* param);
    IContext* getRoot();
    IStream* getIn();
    IStream* getOut();
    IStream* getError();
    void* setBreak(long long increment);
    bool trace(bool on);

    // IRuntime
    void setStartup(void (*startup)(void* (*start)(void* param), void* param));
    void setFocus(void* (*focus)(void* param));

    // IProcess
    void kill();
    void start();
    void start(IFile* file);
    void start(IFile* file, const char* arguments);
    int wait();
    int getExitValue();
    bool hasExited();
    void setRoot(IContext* root);
    void setIn(IStream* in);
    void setOut(IStream* out);
    void setError(IStream* error);

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);

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
    static int set(Process* process, void* object, const Guid& iid);

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

    /** Copies back the input parameters from the server user stack.
     * @return  error number
     */
    int copyOut(UpcallRecord* record);
};

#endif // __es__

#endif // NINTENDO_ES_KERNEL_PROCESS_H_INCLUDED
