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

        bool notify = standbyList.isEmpty();
        standbyList.addLast(page);
        ++standbyCount;
    }
    if (notify)
    {
        PageTable::notify();
    }
}

bool PageSet::
createInstance(const Guid& riid, void** objectPtr)
{
    *objectPtr = 0;
    PageSet* instance = new PageSet(this);
    if (!instance)
    {
        throw SystemException<ENOMEM>();
    }
    bool rc = instance->queryInterface(riid, objectPtr);
    instance->release();
    return rc;
}

bool PageSet::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IPageSet)
    {
        *objectPtr = static_cast<IPageSet*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IClassFactory*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IPageSet*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int PageSet::
addRef(void)
{
    return ref.addRef();
}

unsigned int PageSet::
release(void)
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
        esReport("  %p: cache %p, offset %p, flags %02x, ref %lu\n",
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
        esReport("  %p: cache %p, offset %p, flags %02x, ref %lu\n",
                 page->getPointer(),
                 page->cache,
                 page->getOffset(),
                 page->flags,
                 (unsigned long) page->ref);
        ASSERT(!(page->flags & Page::Changed));
    }
    esReport("\n");
}
