/*
 * Copyright (c) 2006, 2007
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

Page* Cache::
lookupPage(long long offset)
{
    return PageTable::lookup(this, offset);
}

Page* Cache::
getPage(long long offset)
{
    offset &= ~(Page::SIZE - 1);
    for (;;)
    {
        Page* page = 0;

        {
            // If we don't lock this cache here, we could assign
            // more than one page to this cache.
            Monitor::Synchronized method(monitor);

            if (size <= offset)
            {
                return 0;
            }
            page = lookupPage(offset);
            if (page)
            {
                return page;
            }
            page = pageSet->alloc(this, offset);
            if (page)
            {
                return page;
            }
            page = pageSet->steal(this, offset);
            if (page)
            {
                return page;
            }
        }

        flush();

        // Wait until at least one page becomes free.
        PageTable::wait();
    }
}

Page* Cache::
getChangedPage()
{
    Monitor::Synchronized method(monitor);

    Page* page = changedList.getFirst();
    if (page)
    {
        page->addRef();
    }
    return page;
}

Page* Cache::
getStalePage()
{
    Monitor::Synchronized method(monitor);

    PageList::Iterator iter = changedList.begin();
    Page* page;
    while ((page = iter.next()))
    {
        if (PageTable::isLow() || page->isStale())
        {
            page->addRef();
            return page;
        }
    }
    return 0;
}

void Cache::
touch()
{
    DateTime now(DateTime::getNow());
    if (lastUpdated < now)
    {
        lastUpdated = now;
    }
    else
    {
        // For the low resolution timer
        lastUpdated = lastUpdated.addTicks(1);
    }
}

bool Cache::
isStale()
{
    return (DelayedWrite < DateTime::getNow().getTicks() - lastUpdated.getTicks()) ? true : false;
}

bool Cache::
change(Page* page)
{
    Monitor::Synchronized method(monitor);

    bool changed;
    {
        SpinLock::Synchronized method(page->spinLock);
        if (!(page->flags & (Page::Changed | Page::Free)))
        {
            changed = true;
            page->addRef();
            page->flags |= Page::Changed;
            page->touch();

            bool wasEmpty = changedList.isEmpty();
            changedList.addLast(page);
            if (wasEmpty)
            {
                cacheFactory->change(this);
                touch();
            }
        }
        else
        {
            changed = false;
        }
    }
    return changed;
}

bool Cache::
clean(Page* page)
{
    Monitor::Synchronized method(monitor);

    bool changed;
    {
        SpinLock::Synchronized method(page->spinLock);
        if (page->flags & Page::Changed)
        {
            changed = true;
            page->flags &= ~Page::Changed;
            changedList.remove(page);
            page->release();
            if (changedList.isEmpty())
            {
                cacheFactory->clean(this);
            }
        }
        else
        {
            changed = false;
        }
    }
    return changed;
}

int Cache::
read(void* dst, int count, long long offset)
{
    int len;
    int n;

    for (len = 0;
         len < count && offset < size;
         len += n, offset += n, dst = (u8*) dst + n)
    {
        Page* page = getPage(offset);
        if (!page)
        {
            break;
        }

        n = Page::SIZE - Page::pageOffset(offset);
        if (count - len < n)
        {
            n = count - len;
        }
        if (size - offset < n)
        {
            n = size - offset;
        }

        page->fill(backingStore);

        n = page->read(dst, n, Page::pageOffset(offset));
        page->release();
    }
    return len;
}

int Cache::
write(const void* src, int count, long long offset)
{
    int len;
    int n;

    if (size < offset + count)
    {
        setSize(offset + count);
    }

    for (len = 0;
         len < count && offset < size;
         len += n, offset += n, src = (u8*) src + n)
    {
        Page* page = getPage(offset);
        if (!page)
        {
            break;
        }

        n = Page::SIZE - Page::pageOffset(offset);
        if (count - len < n)
        {
            n = count - len;
        }
        if (size - offset < n)
        {
            n = size - offset;
        }

        if (n < Page::SIZE)
        {
            page->fill(backingStore);
        }

        n = page->write(src, n, Page::pageOffset(offset));

        page->filled = true;    // XXX

        page->change();
    }
    return len;
}

long long Cache::
getSize()
{
    return this->size;
}

void Cache::
setSize(long long newSize)
{
    if (newSize < 0)
    {
        throw SystemException<EINVAL>();
    }

    {
        Monitor::Synchronized method(monitor);

        backingStore->setSize(newSize);

        // The following operation can take a lot of time to do
        // if the size is very very large. However we still don't
        // like to lock the page list to scan all the pages.
        long long offset;
        for (offset = (newSize + Page::SIZE - 1) & ~(Page::SIZE - 1);
             offset < size;
             offset += Page::SIZE)
        {
            Page* page = PageTable::lookup(this, offset);
            if (page)
            {
                page->free();
            }
        }
        size = newSize;
    }
}

int Cache::
getSectorSize()
{
    size = this->sectorSize;
    return size;
}

void Cache::
setSectorSize(int size)
{
    this->sectorSize = size;
}

void Cache::
flush()
{
    Page* page;
    while ((page = getChangedPage()))
    {
        page->sync(backingStore, sectorSize);
        page->touch();
        page->release();
    }
    touch();
}

void Cache::
clean()
{
    Page* page;
    while ((page = getStalePage()))
    {
        page->sync(backingStore, sectorSize);
        page->touch();
        page->release();
    }
    touch();
}

unsigned long Cache::
incPageCount()
{
    unsigned long count = pageCount.addRef();
    if (count == 1)
    {
        addRef();
    }
    return count;
}

unsigned long Cache::
decPageCount()
{
    unsigned long count = pageCount.release();
    if (count == 0)
    {
        release();
    }
    return count;
}

unsigned long long Cache::
get(long long offset)
{
    Page* page = getPage(offset);
    if (!page)
    {
        return 0;
    }
    page->fill(backingStore);
    return page->getAddress() | Page::PTEVALID;
}

void Cache::
put(long long offset, unsigned long long pte)
{
    Page* page = PageTable::lookup(pte);
    if (page)
    {
        if (pte & Page::PTEDIRTY)
        {
            page->sync(backingStore, sectorSize);
            page->touch();
        }
        page->release();
    }
}

Cache::
Cache(CacheFactory* cacheFactory, IStream* backingStore, PageSet* pageSet) :
    cacheFactory(cacheFactory),
    backingStore(backingStore),
    file(static_cast<IFile*>(backingStore->queryInterface(IFile::iid()))),
    pageSet(pageSet),
    pageCount(0),
    sectorSize(Page::SIZE)
{
    pageSet->addRef();
    backingStore->addRef();
    size = backingStore->getSize();
    cacheFactory->add(this);
}

Cache::
~Cache()
{
    ASSERT(ref == 0);
    ASSERT(pageCount == 0);
    if (file)
    {
        file->release();
    }
    backingStore->release();
    cacheFactory->remove(this);
    pageSet->release();
}

void Cache::
invalidate()
{
    Monitor::Synchronized method(monitor);

    Page* page;
    while ((page = getChangedPage()))
    {
        page->free();
    }
}

IStream* Cache::
getStream()
{
    Stream* stream = new Stream(this);
    return stream;
}

IStream* Cache::
getInputStream()
{
    InputStream* stream = new InputStream(this);
    return stream;
}

IStream* Cache::
getOutputStream()
{
    OutputStream* stream = new OutputStream(this);
    return stream;
}

unsigned long long Cache::
getPageCount()
{
    return pageCount;
}

void* Cache::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == ICache::iid())
    {
        objectPtr = static_cast<ICache*>(this);
    }
    else if (riid == IPageable::iid())
    {
        objectPtr = static_cast<IPageable*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<ICache*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
}

unsigned int Cache::
addRef(void)
{
    return ref.addRef();
}

unsigned int Cache::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
