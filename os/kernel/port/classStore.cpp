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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <new>
#include "classStore.h"

ClassStore::
ClassStore(int capacity) :
    hashtable(capacity)
{
}

ClassStore::
~ClassStore()
{
    // XXX call release() for all the registered factory classes.
}

void ClassStore::
add(const Guid& clsid, IClassFactory* factory)
{
    if (!factory)
    {
        esThrow(EINVAL);
    }
    {
        SpinLock::Synchronized method(spinLock);

        hashtable.add(clsid, factory);
    }
    factory->addRef();
}

void ClassStore::
remove(const Guid& clsid)
{
    SpinLock::Synchronized method(spinLock);

    IClassFactory* registered = hashtable.get(clsid);
    ASSERT(registered);
    hashtable.remove(clsid);
    registered->release();
}

void* ClassStore::
createInstance(const Guid& rclsid, const Guid& riid)
{
    void* objectPtr = 0;
    IClassFactory* factory;
    {
        SpinLock::Synchronized method(spinLock);

        factory = hashtable.get(rclsid);
        ASSERT(factory);
    }
    // XXX Should ensure 'factory' is valid while calling createInstance().
    return factory->createInstance(riid);
}

void* ClassStore::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == IClassStore::iid())
    {
        objectPtr = static_cast<IClassStore*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<IClassStore*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
}

unsigned int ClassStore::
addRef(void)
{
    return ref.addRef();
}

unsigned int ClassStore::
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
