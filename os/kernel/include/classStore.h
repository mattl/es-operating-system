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
#include <es/hashtable.h>
#include <es/ref.h>
#include <es/base/IClassStore.h>
#include "thread.h"

class ClassStore : public IClassStore
{
    SpinLock    spinLock;
    Ref         ref;
    Hashtable<Guid, IClassFactory*>
                hashtable;

public:
    ClassStore(int capacity = 128);
    ~ClassStore();

    // IClassStore
    void add(const Guid& clsid, IClassFactory* factory);
    void remove(const Guid& clsid);
    void* createInstance(const Guid& rclsid, const Guid& riid);

    // IInterface
    void* queryInterface(const Guid& riid);
    unsigned int addRef(void);
    unsigned int release(void);
};

#endif // NINTENDO_ES_KERNEL_CLASS_STORE_H_INCLUDED
