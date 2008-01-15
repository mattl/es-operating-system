/*
 * Copyright 2008 Google Inc.
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

void* CacheFactory::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == ICacheFactory::iid())
    {
        objectPtr = static_cast<ICacheFactory*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<ICacheFactory*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
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
