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

#include <string.h>
#include <new>
#include <es.h>
#include "cache.h"

SpinLock                PageTable::spinLock __attribute__((init_priority(1000)));       // Before System
void*                   PageTable::base;
size_t                  PageTable::size;
size_t                  PageTable::pageCount;
Page*                   PageTable::pageTable;
PageTable::PageList*    PageTable::hashTable;
PageSet*                PageTable::pageSet;
Monitor                 PageTable::monitor __attribute__((init_priority(1000)));        // Before System

// Note base is specified as a physical address
void PageTable::
init(void* base, size_t size)
{
#ifdef __es__
#ifdef __i386__
    base = (void*) (((unsigned) base) | 0x80000000);
#ifndef NDEBUG
    memset(base, 0xa5, size);   // Fill memory with some non-zero value.
#endif
#endif // __i386__
#endif

    PageTable::base = base;
    PageTable::size = size;
    pageCount = size / Page::SIZE;

    size_t count;

    u8* addr = static_cast<u8*>(base);
    pageTable = reinterpret_cast<Page*>(base);
    Page* page;
    for (page = pageTable, count = 0;
         count < pageCount;
         ++page, ++count, addr += Page::SIZE)
    {
        new(page) Page(addr);
    }

    addr = static_cast<u8*>(base) + sizeof(Page) * pageCount;
    hashTable = reinterpret_cast<PageList*>(addr);
    PageList* list;
    for (list = hashTable, count = 0;
         count < pageCount;
         ++list, ++count)
    {
        new(list) PageList;
        ASSERT(list->isEmpty());
    }

    addr += sizeof(PageList) * pageCount;
    pageSet = reinterpret_cast<PageSet*>(addr);
    new(pageSet) PageSet;

    addr += sizeof(PageSet);
    page = lookup(addr);
    for (++page;
         page < &pageTable[pageCount];
         ++page)
    {
        page->pageSet = pageSet;
        pageSet->freeList.addLast(page);
        ++pageSet->freeCount;
    }
}

void PageTable::
add(Page* page)
{
    SpinLock::Synchronized method(spinLock);

    ASSERT(page->cache);
    hashTable[page->hashCode() % pageCount].addFirst(page);
}

void PageTable::
remove(Page* page)
{
    SpinLock::Synchronized method(spinLock);

    ASSERT(hashTable[page->hashCode() % pageCount].contains(page));
    hashTable[page->hashCode() % pageCount].remove(page);
}

bool PageTable::
steal(Page* page)
{
    SpinLock::Synchronized method(spinLock);

    if (1 < page->addRef())
    {
        page->release();
        return false;
    }

    ASSERT(!(page->flags & Page::Changed));
    hashTable[page->hashCode() % pageCount].remove(page);
    return true;
}

Page* PageTable::
lookup(Cache* cache, long long offset)
{
    SpinLock::Synchronized method(spinLock);

    offset &= ~(Page::SIZE - 1);

    Page* page;
    PageList::Iterator iter =
        hashTable[Page::hashCode(cache, offset) % pageCount].begin();
    while ((page = iter.next()))
    {
        ASSERT(page->cache);
        unsigned long count = page->addRef();
        if (page->cache == cache && page->offset == offset)
        {
            if (count == 1)
            {
                page->pageSet->use(page);
            }
            return page;
        }
        if (count == 1)
        {
            page->pageSet->use(page);
        }
        page->release();
    }
    return 0;
}

Page* PageTable::
lookup(void* addr)
{
    int n = (static_cast<u8*>(addr) - static_cast<u8*>(base)) / Page::SIZE;
    if (n < 0 || pageCount <= n)
    {
        return 0;
    }
    Page* page = &pageTable[n];
    return page;
}

Page* PageTable::
lookup(unsigned long addr)
{
    // XXX 32bit only
    int n = (addr - (reinterpret_cast<unsigned long>(base) & ~0xc0000000)) / Page::SIZE;
    if (n < 0 || pageCount <= n)
    {
        return 0;
    }
    Page* page = &pageTable[n];
    return page;
}

unsigned long long PageTable::
getFreeCount()
{
    return pageSet ? pageSet->getFreeCount() : 0;
}

unsigned long long PageTable::
getStandbyCount()
{
    return pageSet ? pageSet->getStandbyCount() : 0;
}

void PageTable::
report()
{
    size_t count;
    Page* page;

    esReport("\nPageTable::report()\n");
    for (page = pageTable, count = 0;
         count < pageCount;
         ++page, ++count)
    {
        esReport("%p: cache %p, offset 0x%llx, flags %02x, ref %lu\n",
                 page->getPointer(),
                 page->cache,
                 page->getOffset(),
                 page->flags,
                 (unsigned long) page->ref);
    }

    pageSet->report();
}

bool PageTable::
isLow()
{
    return pageSet ? pageSet->isLow() : true;
}

void PageTable::
wait()
{
    Monitor::Synchronized method(monitor);

    monitor.notifyAll();
    while (pageSet->isLow())
    {
        monitor.wait(10000000); // wait for 1 sec
    }
}

void PageTable::
notify()
{
    monitor.notifyAll();
}

void PageTable::
sleep()
{
    Monitor::Synchronized method(monitor);

    if (4 <= pageSet->getFreeCount() + pageSet->getStandbyCount())
    {
        monitor.wait(10000000); // wait for 1 sec
    }
}
