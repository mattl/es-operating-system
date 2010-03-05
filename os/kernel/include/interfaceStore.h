/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_KERNEL_INTERFACE_STORE_H_INCLUDED
#define NINTENDO_ES_KERNEL_INTERFACE_STORE_H_INCLUDED

#include <es.h>
#include <es/any.h>
#include <es/hashtable.h>
#include <es/ref.h>
#include <es/reflect.h>
#include <es/includeAllInterfaces.h>
#include "thread.h"

class InterfaceStore : public es::InterfaceStore
{
    struct CompareName
    {
        bool operator() (const char* a, const char* b) const
        {
            return (a == b) || (strcmp(a, b) == 0);
        }
    };

    struct MetaData
    {
        Reflect::Interface meta;
        Object* (*constructorGetter)();                 // for statically created data
        void (*constructorSetter)(Object* constructor); // for statically created data
        Object* constructor;                            // for dynamically created data

        MetaData() :
            constructorGetter(0),
            constructorSetter(0),
            constructor(0)
        {
        }

        MetaData(const char* info, const char* iid) :
            meta(info, iid),
            constructorGetter(0),
            constructorSetter(0),
            constructor(0)
        {
        }

        Object* getConstructor() const
        {
            if (constructor) {
                return constructor;
            }
            if (constructorGetter) {
                return constructorGetter();
            }
            return 0;
        }
    };

    SpinLock spinLock;
    Ref ref;
    Hashtable<const char*, MetaData, 1024, Hash<const char*>, CompareName> hashtable;

    void registerConstructor(const char* iid, Object* (*getter)(), void (*setter)(Object*));
    void updateInheritedMethodCount(const char* iid);

public:
    InterfaceStore();
    ~InterfaceStore();

    Reflect::Interface& getInterface(const char* iid)
    {
        SpinLock::Synchronized method(spinLock);

        return hashtable.get(iid).meta;
    }

    Object* getConstructor(const char* iid)
    {
        SpinLock::Synchronized method(spinLock);

        return hashtable.get(iid).getConstructor();
    }

    bool hasInterface(const char* iid)
    {
        SpinLock::Synchronized method(spinLock);

        return hashtable.contains(iid);
    }

    const char* getUniqueIdentifier(const char* iid)
    {
        SpinLock::Synchronized method(spinLock);

        try
        {
            return hashtable.get(iid).meta.getMetaData();
        }
        catch (...)
        {
            return 0;
        }
    }

    // IInterfaceStore
    void add(const char* iid, const char* info);
    void remove(const char* iid);

    // IInterface
    Object* queryInterface(const char* iid);
    unsigned int addRef();
    unsigned int release();
};

namespace es
{
    Reflect::Interface& getInterface(const char* iid);
    Object* getConstructor(const char* iid);
    const char* getUniqueIdentifier(const char* iid);
}  // namespace es

#endif // NINTENDO_ES_KERNEL_INTERFACE_STORE_H_INCLUDED
