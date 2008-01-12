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
#include <es/base/IClassFactory.h>
#include "cache.h"

void CacheFactory::
add(Cache* cache)
{
    addRef();

    {
        SpinLock::Synchronized method(spinLock);

        standbyList.addLast(cache);
    }
}

void CacheFactory::
remove(Cache* cache)
{
    {
        SpinLock::Synchronized method(spinLock);

        standbyList.remove(cache);
    }

    release();
}

void CacheFactory::
change(Cache* cache)
{
    SpinLock::Synchronized method(spinLock);

    standbyList.remove(cache);
    changedList.addLast(cache);
}

void CacheFactory::
clean(Cache* cache)
{
    SpinLock::Synchronized method(spinLock);

    changedList.remove(cache);
    standbyList.addLast(cache);
}

Cache* CacheFactory::
getStaleCache()
{
    SpinLock::Synchronized method(spinLock);

    CacheList::Iterator iter = changedList.begin();
    Cache* cache;
    while ((cache = iter.next()))
    {
        if (PageTable::isLow() || cache->isStale())
        {
            cache->addRef();
            return cache;
        }
    }
    return 0;
}

void CacheFactory::
update()
{
    while (1 < ref)
    {
        PageTable::sleep();

        Cache* cache;
        while ((cache = getStaleCache()))
        {
            if (cache->monitor.tryLock())
            {
                cache->clean();
                cache->monitor.unlock();
            }
            else
            {
                // We should fix this implementation so that update() does not
                // lock up the system.
                Thread::reschedule();
            }
            cache->release();
        }
    }
    release();  // ref is incremenet for the thread executing this method
}

void* CacheFactory::
run(void* param)
{
    CacheFactory* cacheFactory;

    cacheFactory = static_cast<CacheFactory*>(param);
    cacheFactory->update();
    return 0;
}

CacheFactory::
CacheFactory() :
    thread(run, this, IThread::Normal)
{
    addRef();   // for thread

    thread.start();
}

CacheFactory::
~CacheFactory()
{
    ASSERT(ref == 0);
    ASSERT(standbyList.isEmpty());
    ASSERT(changedList.isEmpty());
}

ICache* CacheFactory::
create(IStream* backingStore)
{
    return new Cache(this, backingStore, PageTable::pageSet);
}

ICache* CacheFactory::
create(IStream* backingStore, IPageSet* pageSet)
{
    PageSet* ps = dynamic_cast<PageSet*>(pageSet);
    if (!ps)
    {
        esThrow(EINVAL);
    }
    return new Cache(this, backingStore, ps);
}

bool CacheFactory::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_ICacheFactory)
    {
        *objectPtr = static_cast<ICacheFactory*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<ICacheFactory*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int CacheFactory::
addRef(void)
{
    return ref.addRef();
}

unsigned int CacheFactory::
release(void)
{
    unsigned int count = ref.release();
    switch (count)
    {
      case 1:
        PageTable::notify();    // To wake up the update thread
        break;
      case 0:
        delete this;
        break;
    }
    return count;
}
