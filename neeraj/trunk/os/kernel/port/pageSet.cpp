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

#include <new>
#include <errno.h>
#include <es.h>
#include <es/exception.h>
#include "cache.h"

PageSet::
PageSet(PageSet* parent) :
    parent(parent),
    freeCount(0),
    standbyCount(0)
{
    if (parent)
    {
        parent->addRef();
    }
}

PageSet::
~PageSet()
{
    ASSERT(standbyCount == 0);
    if (!parent)
    {
        return;
    }
    while (0 < freeCount)
    {
        Page* page = alloc();
        {
            SpinLock::Synchronized method(spinLock);
            page->pageSet = parent;
            page->flags |= Page::Free;
        }
        page->release();
        {
            SpinLock::Synchronized method(parent->spinLock);

            parent->freeList.addLast(page);
            ++parent->freeCount;
        }
    }
    parent->release();
}

void PageSet::
reserve(unsigned long long reserveCount)
{
    if (!parent)
    {
        esThrow(EINVAL);
    }
    while (0 < reserveCount--)
    {
        Page* page = parent->alloc();
        if (!page)
        {
            page = parent->steal();
        }
        if (page)   // XXX
        {
            {
                SpinLock::Synchronized method(spinLock);

                page->pageSet = this;
                page->flags |= Page::Free;
            }
            page->release();
        }
    }
}

unsigned long long PageSet::
getFreeCount()
{
    return freeCount;
}

unsigned long long PageSet::
getStandbyCount()
{
    return standbyCount;
}

Page* PageSet::
alloc()
{
    SpinLock::Synchronized method(spinLock);

    Page* page = freeList.removeFirst();
    if (page)
    {
        --freeCount;
        page->addRef();
    }
    return page;
}

Page* PageSet::
steal()
{
    SpinLock::Synchronized method(spinLock);

    // Try to steal a page from the stand-by list. Note if
    // a page is referenced, it must not be stolen from the list.
    Page* page;
    PageList::Iterator iter(standbyList.begin());
    while ((page = iter.next()))
    {
        ASSERT(!(page->flags & Page::Changed));
        if (PageTable::steal(page))
        {
            // We assume page->cache is a valid pointer inside this method,
            // which is guaranteed by page->cache field is not modified
            // while the reference count is greater than zero.
            --standbyCount;
            iter.remove();

            Cache* cache = page->cache;
            page->cache = 0;
            cache->decPageCount();
            break;
        }
    }
    return page;
}

bool PageSet::
isLow()
{
    SpinLock::Synchronized method(spinLock);

    return (freeList.isEmpty() && standbyList.isEmpty()) ? true : false;
}

// We assume no page is allocated to this cache at this offset.
Page* PageSet::
alloc(Cache* cache, long long offset)
{
    offset &= ~(Page::SIZE - 1);

    PageSet* pageSet;
    for (pageSet = this; pageSet; pageSet = pageSet->parent)
    {
        Page* page = pageSet->alloc();
        if (page)
        {
            page->set(cache, offset);
            return page;
        }
    }
    return 0;
}

Page* PageSet::
steal(Cache* cache, long long offset)
{
    offset &= ~(Page::SIZE - 1);

    PageSet* pageSet;
    for (pageSet = this; pageSet; pageSet = pageSet->parent)
    {
        Page* page = pageSet->steal();
        if (page)
        {
            page->set(cache, offset);
            return page;
        }
    }
    return 0;
}

void PageSet::
use(Page* page)
{
    SpinLock::Synchronized method(spinLock);

    --standbyCount;
    standbyList.remove(page);
    ASSERT(!standbyList.contains(page));
}

void PageSet::
free(Page* page)
{
    ASSERT(page->pageSet == this);
    bool notify(false);
    {
        SpinLock::Synchronized method(spinLock);

        Cache* cache = page->cache;
        if (cache)
        {
            cache->decPageCount();
        }

        page->cache = 0;
        page->offset = 0;
        page->flags = 0;

        notify = freeList.isEmpty();
        freeList.addLast(page);
        ++freeCount;
    }
    if (notify)
    {
        PageTable::notify();
    }
}

void PageSet::
standby(Page* page)
{
    ASSERT(page->pageSet == this);
    ASSERT(!(page->flags & Page::Changed));
    ASSERT(!standbyList.contains(page));
    bool notify(false);
    {
        SpinLock::Synchronized method(spinLock);

        standbyList.addLast(page);
        ++standbyCount;
    }
    if (notify)
    {
        PageTable::notify();
    }
}

es::PageSet* PageSet::
fork()
{
    PageSet* instance = new PageSet(this);
    if (!instance)
    {
        throw SystemException<ENOMEM>();
    }
   return instance;
}

Object* PageSet::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::PageSet::iid()) == 0)
    {
        objectPtr = static_cast<es::PageSet*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::PageSet*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int PageSet::
addRef()
{
    return ref.addRef();
}

unsigned int PageSet::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
    }
    return count;
}

void PageSet::
report()
{
    Page* page;

    esReport("\nfreeList:\n");
    PageList::Iterator iterFree(freeList.begin());
    while ((page = iterFree.next()))
    {
        esReport("  %p: cache %p, offset 0x%llx, flags %02x, ref %lu\n",
                 page->getPointer(),
                 page->cache,
                 page->getOffset(),
                 page->flags,
                 (unsigned long) page->ref);
        ASSERT(!(page->flags & Page::Changed));
    }
    esReport("\n");

    esReport("\nstandbyList:\n");
    PageList::Iterator iter(standbyList.begin());
    while ((page = iter.next()))
    {
        esReport("  %p: cache %p, offset 0x%llx, flags %02x, ref %lu\n",
                 page->getPointer(),
                 page->cache,
                 page->getOffset(),
                 page->flags,
                 (unsigned long) page->ref);
        ASSERT(!(page->flags & Page::Changed));
    }
    esReport("\n");
}

es::PageSet* PageSet::
Constructor::createInstance()
{
    ASSERT(PageTable::pageSet);
    return PageTable::pageSet->fork();
}

Object* PageSet::
Constructor::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::PageSet::Constructor::iid()) == 0)
    {
        objectPtr = static_cast<es::PageSet::Constructor*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::PageSet::Constructor*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int PageSet::
Constructor::addRef()
{
    return 1;
}

unsigned int PageSet::
Constructor::release()
{
    return 1;
}

void PageSet::
initializeConstructor()
{
    // cf. -fthreadsafe-statics for g++
    static Constructor constructor;
    es::PageSet::setConstructor(&constructor);
}
