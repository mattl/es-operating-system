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

#include <es/ref.h>
#include <es/list.h>
#include <es/base/IProcess.h>
#include "cache.h"
#include "thread.h"
#include "zero.h"

class InterfaceDescriptor;
class Map;
class Process;

class InterfaceStub
{
    Ref         ref;
    void*       interface;
    const Guid* iid;

public:
    InterfaceStub() :
        ref(0),
        interface(0),
        iid(0)
    {
    }

    int set(void* interface, const Guid* iid)
    {
        if (this->interface != 0)
        {
            return -1;
        }

        ASSERT(this->iid == 0);
        this->interface = interface;
        this->iid = iid;
        addRef();
        return 0;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            static_cast<IInterface*>(interface)->release();
            interface = 0;
            iid = 0;
            return 0;
        }
        return count;
    }

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

class Process : public IProcess
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
    InterfaceStub   interfaceTable[INTERFACE_POINTER_MAX];

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

    int condWait(int);

public:
    Process();
    ~Process();

    void load();

    Map* lookup(const void* addr);
    bool isValid(const void* start, long long length, bool write);
    void dump();

    int validityFault(const void* addr, u32 error);
    int protectionFault(const void* addr, u32 error);

    long long systemCall(void** self, unsigned methodNumber, void* stackPointer, void** base);
    int set(void* interface, const Guid* iid);

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
    void setStartup(void (*startup)(void* (*start)(void* param), void* param));

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

    friend class Mmu;
    friend class Swap;
};

#endif // __es__

//
// Reflection data of the default interface set
//

extern unsigned char IAlarmInfo[];
extern unsigned char ICacheInfo[];
extern unsigned char ICallbackInfo[];
extern unsigned char IClassFactoryInfo[];
extern unsigned char IClassStoreInfo[];
extern unsigned char IFileInfo[];
extern unsigned char IInterfaceInfo[];
extern unsigned char IMonitorInfo[];
extern unsigned char IPageableInfo[];
extern unsigned char IPageSetInfo[];
extern unsigned char IProcessInfo[];
extern unsigned char IStreamInfo[];
extern unsigned char IThreadInfo[];

extern unsigned char IAudioFormatInfo[];
extern unsigned char IBeepInfo[];
extern unsigned char ICursorInfo[];
extern unsigned char IDeviceInfo[];
extern unsigned char IDiskManagementInfo[];
extern unsigned char IDmacInfo[];
extern unsigned char IFileSystemInfo[];
extern unsigned char IPicInfo[];
extern unsigned char IRemovableMediaInfo[];
extern unsigned char IRtcInfo[];
extern unsigned char IPartitionInfo[];

extern unsigned char IBindingInfo[];
extern unsigned char IContextInfo[];

extern unsigned char IIteratorInfo[];
extern unsigned char ISetInfo[];

#endif // NINTENDO_ES_KERNEL_PROCESS_H_INCLUDED
