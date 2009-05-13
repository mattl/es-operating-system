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
#include "cache.h"

void Cache::Constructor::
add(Cache* cache)
{
    addRef();

    {
        SpinLock::Synchronized method(spinLock);

        standbyList.addLast(cache);
    }
}

void Cache::Constructor::
remove(Cache* cache)
{
    {
        SpinLock::Synchronized method(spinLock);

        standbyList.remove(cache);
    }

    release();
}

void Cache::Constructor::
change(Cache* cache)
{
    SpinLock::Synchronized method(spinLock);

    standbyList.remove(cache);
    changedList.addLast(cache);
}

void Cache::Constructor::
clean(Cache* cache)
{
    SpinLock::Synchronized method(spinLock);

    changedList.remove(cache);
    standbyList.addLast(cache);
}

Cache* Cache::Constructor::
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

void Cache::Constructor::
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

void* Cache::Constructor::
run(void* param)
{
    Cache::Constructor* cacheFactory;

    cacheFactory = static_cast<Cache::Constructor*>(param);
    cacheFactory->update();
    return 0;
}

Cache::
Constructor::Constructor() :
    thread(run, this, es::Thread::Normal)
{
    addRef();   // for thread

    thread.start();
}

Cache::
Constructor::~Constructor()
{
    ASSERT(ref == 0);
    ASSERT(standbyList.isEmpty());
    ASSERT(changedList.isEmpty());
}

es::Cache* Cache::
Constructor::createInstance(es::Stream* backingStore)
{
    return new Cache(this, backingStore, PageTable::pageSet);
}

es::Cache* Cache::
Constructor::createInstance(es::Stream* backingStore, es::PageSet* pageSet)
{
    PageSet* ps = dynamic_cast<PageSet*>(pageSet);
    if (!ps)
    {
        esThrow(EINVAL);
    }
    return new Cache(this, backingStore, ps);
}

Object* Cache::
Constructor::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Cache::Constructor::iid()) == 0)
    {
        objectPtr = static_cast<es::Cache::Constructor*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Cache::Constructor*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Cache::
Constructor::addRef()
{
    return ref.addRef();
}

unsigned int Cache::
Constructor::release()
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

void Cache::
initializeConstructor()
{
    // cf. -fthreadsafe-statics for g++
    Constructor* constructor = new Constructor;
    es::Cache::setConstructor(constructor);
}
