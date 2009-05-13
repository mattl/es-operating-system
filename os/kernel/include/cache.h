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

#ifndef NINTENDO_ES_KERNEL_CACHE_H_INCLUDED
#define NINTENDO_ES_KERNEL_CACHE_H_INCLUDED

#include "thread.h"
#include <es.h>
#include <es/list.h>
#include <es/dateTime.h>
#include <es/base/ICache.h>
#include <es/base/IFile.h>
#include <es/base/IPageable.h>
#include <es/base/IPageSet.h>
#include <es/base/IStream.h>

class Cache;
class CacheFactory;
class Page;
class PageSet;
class PageTable;
class Stream;

class Mmu;
class Process;
class Swap;

class Page
{
    SpinLock            spinLock;
    Ref                 ref;
    Cache*              cache;
    CacheFactory*       cacheFactory;
    long long           offset;
    Link<Page>          linkHash;       // for hashTable
    Link<Page>          linkChain;      // for pageSet
    volatile unsigned   flags;
    void*               pointer;
    PageSet*            pageSet;
    u64                 map;            // per sector modified map

    Monitor             monitor;        // for Filled
    bool                filled;

    DateTime            lastUpdated;

    enum
    {
        // flags
        Changed = 0x01,
        Referenced = 0x02,
        Free = 0x04
    };

    Page(void* pointer);

    int hashCode() const
    {
        return hashCode(cache, offset);
    }

    long long getOffset() const
    {
        return offset;
    }

    void* getPointer() const
    {
        return pointer;
    }

    unsigned long getAddress() const
    {
#ifdef __i386__
        return reinterpret_cast<unsigned long>(pointer) & ~0xc0000000;
#endif // __i386__
    }

    void set(Cache* cache, long long offset);

    int fill(es::Stream* backingStore);
    int sync(es::Stream* backingStore, int sectorSize);

    /** Updates lastUpdated to the current time.
     */
    void touch();

    /** Checks if this cache is stale or not. Not that this
     * function must not wait for locking the monitor.
     */
    bool isStale();

    void free();
    void change();

    int read(void* dst, int count, long long offset);
    int write(const void* src, int count, long long offset);

    unsigned int addRef();
    unsigned int release();

public:
    static const int SIZE;      // Page size in bytes
    static const int SHIFT;     // Number of page offset bits
    static const int SECTOR;    // Sector size in bytes

    // Bits for page table entry
    static const unsigned PTEVALID = 1u<<0;
    static const unsigned PTEWRITE = 1u<<1;
    static const unsigned PTEUSER = 1u<<2;
    static const unsigned PTETHROUGH = 1u<<3;       // write through - i486 or later
    static const unsigned PTEUNCACHED = 1u<<4;      // cache disable - i486 or later
    static const unsigned PTEACCESSED = 1u<<5;
    static const unsigned PTEDIRTY = 1u<<6;
    static const unsigned PTEGLOBAL = 1u<<8;        // global page - Pentium Pro or later
    static const unsigned PTEPRIVATE = 1u<<9;

    static int hashCode(Cache* cache, long long offset)
    {
        int code = (int) (offset >> SHIFT);
        code ^= reinterpret_cast<long>(cache);
        return code;
    }

    static unsigned long pageOffset(unsigned long offset)
    {
        return offset & (SIZE - 1);
    }

    static unsigned long pageBase(unsigned long pte)
    {
        return pte & ~(SIZE - 1);
    }

    static void* round(const void* ptr)
    {
        unsigned long addr(reinterpret_cast<unsigned long>(ptr));
        addr += SIZE - 1;
        addr &= ~(SIZE - 1);
        return reinterpret_cast<void*>(addr);
    }

    static void* trunc(const void* ptr)
    {
        unsigned long addr(reinterpret_cast<unsigned long>(ptr));
        addr &= ~(SIZE - 1);
        return reinterpret_cast<void*>(addr);
    }

    friend class Cache;
    friend class CacheFactory;
    friend class PageSet;
    friend class PageTable;
    friend class Stream;

    friend class Mmu;
    friend class Process;
    friend class Swap;
};

/** Exposes static methods for adding, removing, and looking up
 * the global page hash table.
 */
class PageTable
{
    typedef List<Page, &Page::linkHash>     PageList;

    static SpinLock     spinLock;
    static void*        base;
    static size_t       size;
    static size_t       pageCount;  // XXX make this long long breaks the kernel...
    static Page*        pageTable;
    static PageList*    hashTable;
    static PageSet*     pageSet;     // default page set

    static Monitor      monitor;

public:
    static void init(void* base, size_t size);

    /** Adds this page to the hash table.
     */
    static void add(Page* page);

    /** Removes this page from the hash table.
     */
    static void remove(Page* page);

    /** Tries to steal this page from the hash table. If this
     * page is referenced, it is not stolen from the hash table.
     * @return  true if this page is stolen successfully.
     */
    static bool steal(Page* page);

    /** Searches the page for this cache at the specified offset.
     * @return  locked page. NULL if not found.
     */
    static Page* lookup(Cache* cache, long long offset);

    /** Searches the page at the specified address.
     * @return  NULL if not exists.
     */
    static Page* lookup(void* addr);

    /** Searches the page at the specified physical address.
     * @return  NULL if not exists.
     */
    static Page* lookup(unsigned long addr);

    /** Gets the number of free pages
     * @return  free page count
     */
    static unsigned long long getFreeCount();

    /** Gets the number of standby pages
     * @return  free standby count
     */
    static unsigned long long getStandbyCount();

    static bool isLow();

    /** Waits until at least one page becomes allocatable
     */
    static void wait();

    static void notify();

    static void sleep();

    static void report();

    friend class Cache;
    friend class CacheFactory;
    friend class Page;
    friend class PageSet;
    friend class Stream;

    friend int esInit(Object** nameSpace);
};

class PageSet : public es::PageSet
{
    typedef List<Page, &Page::linkChain>    PageList;

    SpinLock        spinLock;
    Ref             ref;
    PageSet*        parent;
    PageList        freeList;
    PageList        standbyList;
    unsigned long   freeCount;
    unsigned long   standbyCount;

    PageSet(PageSet* parent = 0);

    ~PageSet();

    /** Gets a locked page from freeList and set it to the cache.
     * @return  locked page. NULL if no page is free.
     */
    Page* alloc(Cache* cache, long long offset);

    /** Steals a locked page from standbyList and set it to the cache.
     * @return  locked page. NULL if no page is in standbyList.
     */
    Page* steal(Cache* cache, long long offset);

    /** Remove this page from standbyList
     */
    void use(Page* page);

    /** Releases the locked page to freeList
     */
    void free(Page* page);

    /** Moves this page to standbyList.
     */
    void standby(Page* page);

    /** Returns true under a low memory condition
     */
    bool isLow();

    /** Gets the number of free pages
     * @return  free page count
     */
    unsigned long long getFreeCount();

    /** Gets the number of standby pages
     * @return  free standby count
     */
    unsigned long long getStandbyCount();

    /** Gets a locked page from freeList
     * @return  locked page. NULL if no page is free.
     */
    Page* alloc();

    /** Steals a locked page from standbyList
     * @return  locked page. NULL if no page is in standbyList.
     */
    Page* steal();

    void report();

public:
    // IPageSet
    /** Creates a new PageSet from this page set.
     */
    es::PageSet* fork();

    /** Reserves the reserveCount pages from parent.
     */
    void reserve(unsigned long long reserveCount);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    friend class Cache;
    friend class CacheFactory;
    friend class Page;
    friend class PageTable;
    friend class Stream;

    // [Constructor]
    class Constructor : public es::PageSet::Constructor
    {
    public:
        es::PageSet* createInstance();
        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();
    };

    static void initializeConstructor();
};

class Cache : public es::Cache, public es::Pageable
{
    Monitor             monitor;
    Ref                 ref;
    Link<Cache>         link;

public:
    class Constructor : public es::Cache::Constructor
    {
        typedef List<Cache, &Cache::link>       CacheList;

        Ref         ref;

        SpinLock    spinLock;
        CacheList   standbyList;
        CacheList   changedList;
        Thread      thread;

        void add(Cache* cache);
        void remove(Cache* cache);
        void change(Cache* cache);
        void clean(Cache* cache);

        Cache* getStaleCache();

        void update();

        static void* run(void* param);

    public:
        Constructor();
        ~Constructor();

        es::Cache* createInstance(es::Stream* backingStore);
        es::Cache* createInstance(es::Stream* backingStore, es::PageSet* pageSet);

        // IInterface
        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();

        friend class Cache;
        friend class Page;
        friend class PageSet;
        friend class PageTable;
        friend class Stream;
    };

private:
    typedef List<Page, &Page::linkChain>    PageList;

    static const s64 DelayedWrite = 150000000;  // 15 [sec]

    Constructor*        cacheFactory;
    es::Stream*        backingStore;
    es::File*          file;
    PageSet*            pageSet;
    long long           size;
    PageList            changedList;
    Ref                 pageCount;
    int                 sectorSize;

    DateTime            lastUpdated;

    /** Looks up a page at the specified offset.
     * @return  locked page if exists. The reference count of the page is
     *          incremented by one.
     */
    Page* lookupPage(long long offset);

    /** Gets a page at the specified offset.
     * @return  locked page The reference count of the page is
     *          incremented by one.
     */
    Page* getPage(long long offset);

    /** Gets a locked page which is changed.
     * @return  locked page. NULL if no page is changed.
     */
    Page* getChangedPage();

    /** Gets a locked page which is changed and stale.
     * @return  locked page. NULL if no page is stale.
     */
    Page* getStalePage();

    /** Updates lastUpdated to the current time.
     */
    void touch();

    /** Checks if this cache is stale or not. Not that this
     * function must not wait for locking the monitor.
     */
    bool isStale();

    /** Moves this page to changedList.
     * @return  true if this page is just added to changedList.
     */
    bool change(Page* page);

    /** Removes this page from changedList.
     * @return  true if this page has been changed.
     */
    bool clean(Page* page);

    int read(void* dst, int count, long long offset);
    int write(const void* src, int count, long long offset);

    /** Writes back all the changed pages to the backing store.
     */
    void flush();

    /** Writes back stale changed pages to the backing store.
     */
    void clean();

    unsigned long incPageCount();
    unsigned long decPageCount();

public:
    Cache(Cache::Constructor* cacheFactory, es::Stream* backingStore, PageSet* pageSet);
    ~Cache();

    // IPageable
    unsigned long long get(long long offset);
    void put(long long offset, unsigned long long pte);

    // ICache

    /** Creates a new stream for this cache.
     * @return  IStream interface pointer
     */
    es::Stream* getStream();
    es::Stream* getInputStream();
    es::Stream* getOutputStream();

    long long getSize();

    void setSize(long long size);

    int getSectorSize();

    void setSectorSize(int size);

    /** Releases all the changed pages associated with this cache
     * without flushing them.
     */
    void invalidate();

    /** Gets the number of pages associated to this cache.
     * @return  page count
     */
    unsigned long long getPageCount();

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    friend class Constructor;
    friend class Page;
    friend class PageSet;
    friend class PageTable;
    friend class Stream;

    friend class Mmu;
    friend class Process;
    friend class Swap;

    static void initializeConstructor();
};

class Stream : public es::Stream, public es::File
{
    Ref         ref;
    Cache*      cache;

    Monitor     monitor;    // Locks position.
    long long   position;

public:
    Stream(Cache* cache);
    virtual ~Stream();
    long long getPosition();
    void setPosition(long long pos);
    long long getSize();
    void setSize(long long size);
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();

    unsigned int getAttributes();
    void setAttributes(unsigned int attributes);
    long long getCreationTime();
    void setCreationTime(long long creationTime);
    long long getLastAccessTime();
    void setLastAccessTime(long long lastAccessTime);
    long long getLastWriteTime();
    void setLastWriteTime(long long lastWriteTime);
    bool canRead();
    bool canWrite();
    bool isDirectory();
    bool isFile();
    bool isHidden();
    const char* getName(void* name, int nameLength);
    es::Pageable* getPageable();
    es::Stream* getStream();

    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    friend class Cache;
};

class InputStream : public Stream
{
public:
    InputStream(Cache* cache);
    virtual ~InputStream();
    void setSize(long long size);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();
};

class OutputStream : public Stream
{
public:
    OutputStream(Cache* cache);
    virtual ~OutputStream();
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
};

#endif // NINTENDO_ES_KERNEL_CACHE_H_INCLUDED
