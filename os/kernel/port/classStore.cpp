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

bool ClassStore::
createInstance(const Guid& rclsid, const Guid& riid, void** objectPtr)
{
    *objectPtr = 0;
    IClassFactory* factory;
    {
        SpinLock::Synchronized method(spinLock);

        factory = hashtable.get(rclsid);
        ASSERT(factory);
    }
    // XXX Should ensure 'factory' is valid while calling createInstance().
    return factory->createInstance(riid, objectPtr);
}

bool ClassStore::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IClassStore)
    {
        *objectPtr = static_cast<IClassStore*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IClassStore*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
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
