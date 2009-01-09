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
    void* createInstance(const Guid& rclsid, const char* riid);

    // IInterface
    void* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

#endif // NINTENDO_ES_KERNEL_CLASS_STORE_H_INCLUDED
