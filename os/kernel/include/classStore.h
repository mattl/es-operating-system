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

#ifndef NINTENDO_ES_KERNEL_CLASS_STORE_H_INCLUDED
#define NINTENDO_ES_KERNEL_CLASS_STORE_H_INCLUDED

#include <es.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/base/IClassStore.h>
#include "thread.h"

class ClassEntry;
class ClassStore;

class ClassEntry
{
    Guid                clsid;
    IClassFactory*      factory;
    Link<ClassEntry>    link;

    friend class ClassStore;

    ClassEntry() : factory(0)
    {
    }
};

class ClassStore : public IClassStore
{
    typedef List<ClassEntry, &ClassEntry::link> ClassList;

    SpinLock            spinLock;
    Ref                 ref;
    int                 entryCount;
    ClassEntry*         entryTable;
    ClassList*          hashTable;
    ClassList           freeList;

    ClassEntry* lookup(const Guid& clsid);

public:
    ClassStore(int entryCount = 100);
    ~ClassStore();

    // IClassStore
    void add(const Guid& clsid, IClassFactory* factory);
    void remove(const Guid& clsid, IClassFactory* factory);
    bool createInstance(const Guid& rclsid, const Guid& riid, void** objectPtr);

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);
};

#endif // NINTENDO_ES_KERNEL_CLASS_STORE_H_INCLUDED
