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

static int hashCode(const Guid& clsid)
{
    int code = ((u32*) &clsid)[0] ^
               ((u32*) &clsid)[1] ^
               ((u32*) &clsid)[2] ^
               ((u32*) &clsid)[3];
    return code;
}

ClassEntry* ClassStore::
lookup(const Guid& clsid)
{
    ClassEntry* entry;
    ClassList::Iterator iter =
        hashTable[hashCode(clsid) % entryCount].begin();
    while ((entry = iter.next()))
    {
        if (entry->clsid == clsid)
        {
            return entry;
        }
    }
    return 0;
}

ClassStore::
ClassStore(int entryCount) :
    entryCount(entryCount)
{
    entryTable = new ClassEntry[entryCount];
    hashTable = new ClassList[entryCount];

    ClassEntry* entry;
    for (entry = entryTable; entry < &entryTable[entryCount]; ++entry)
    {
        freeList.addLast(entry);
    }
}

ClassStore::
~ClassStore()
{
    ClassEntry* entry;
    for (entry = entryTable; entry < &entryTable[entryCount]; ++entry)
    {
        if (entry->factory)
        {
            entry->factory->release();
        }
    }
    delete entryTable;
    delete hashTable;
}

void ClassStore::
add(const Guid& clsid, IClassFactory* factory)
{
    if (!factory)
    {
        esThrow(EINVAL);
    }
    factory->addRef();

    ClassEntry* entry;
    IClassFactory* prev = 0;
    {
        SpinLock::Synchronized method(spinLock);

        entry = lookup(clsid);
        if (entry)
        {
            prev = entry->factory;
            entry->factory = factory;
        }
        else
        {
            entry = freeList.removeFirst();
            if (entry)
            {
                entry->clsid = clsid;
                entry->factory = factory;
                hashTable[hashCode(clsid) % entryCount].addFirst(entry);
            }
        }
    }

    if (prev)
    {
        prev->release();
    }
    if (!entry)
    {
        factory->release();
        esThrow(ENOSPC);
    }
}

void ClassStore::
remove(const Guid& clsid, IClassFactory* factory)
{
    {
        SpinLock::Synchronized method(spinLock);

        ClassEntry* entry = lookup(clsid);
        if (entry)
        {
            factory = entry->factory;
            entry->factory = 0;
            hashTable[hashCode(clsid) % entryCount].remove(entry);
            freeList.addLast(entry);
        }
        else
        {
            factory = 0;
        }
    }
    if (factory)
    {
        factory->release();
    }
}

bool ClassStore::
createInstance(const Guid& rclsid, const Guid& riid, void** objectPtr)
{
    *objectPtr = 0;

    IClassFactory* factory;
    {
        SpinLock::Synchronized method(spinLock);

        ClassEntry* entry = lookup(rclsid);
        if (!entry)
        {
            esThrow(ENOENT);
        }
        factory = entry->factory;
        factory->addRef();
    }
    bool rc = factory->createInstance(riid, objectPtr);
    factory->release();
    return rc;
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
