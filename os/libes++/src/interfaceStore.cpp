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

#include <stdio.h>
#include <string.h>
#include <es/any.h>
#include <es/hashtable.h>
#include <es/reflect.h>
#include <es/includeAllInterfaces.h>
#include <es/interfaceData.h>

namespace es
{

class InterfaceDataStore
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
            if (constructor)
            {
                return constructor;
            }
            if (constructorGetter)
            {
                return constructorGetter();
            }
            return 0;
        }

        void setConstructor(Object* constructor)
        {
            if (constructorSetter)
            {
                constructorSetter(constructor);
                return;
            }
            this->constructor = constructor;
        }
    };

    Hashtable<const char*, MetaData, 1024, Hash<const char*>, CompareName> hashtable;

public:
    InterfaceDataStore();

    Reflect::Interface& getInterface(const char* iid)
    {
        return hashtable.get(iid).meta;
    }

    Object* getConstructor(const char* iid)
    {
        try
        {
            return hashtable.get(iid).getConstructor();
        }
        catch (...)
        {
            return 0;
        }
    }

    bool hasInterface(const char* iid)
    {
        return hashtable.contains(iid);
    }

    void registerConstructor(const char* iid, Object* (*getter)(), void (*setter)(Object*))
    {
        try
        {
            MetaData* data = &hashtable.get(iid);
            data->constructorGetter = getter;
            data->constructorSetter = setter;
        }
        catch (...)
        {
        }
    }

    void registerConstructor(const char* iid, Object* constructor)
    {
        try
        {
            hashtable.get(iid).setConstructor(constructor);
        }
        catch (...)
        {
        }
    }

    const char* getUniqueIdentifier(const char* iid)
    {
        try
        {
            return hashtable.get(iid).meta.getMetaData();
        }
        catch (...)
        {
            return 0;
        }
    }
};

InterfaceDataStore::
InterfaceDataStore()
{
    for (es::InterfaceData* data = es::interfaceData; data->iid; ++data)
    {
        MetaData metaData(data->info(), data->iid());
        hashtable.add(data->iid(), metaData);
    }

    // Update inheritedMethodCount of each interface data.
    for (es::InterfaceData* data = es::interfaceData; data->iid; ++data)
    {
        unsigned inheritedMethodCount = 0;
        Reflect::Interface* interface = &getInterface(data->iid());
        Reflect::Interface* super = interface;
        for (;;)
        {
            char qualifiedName[256];
            unsigned length;
            const char* superName = super->getQualifiedSuperName(&length);
            if (!superName)
            {
                break;
            }
            ASSERT(length < sizeof qualifiedName);
            memcpy(qualifiedName, superName, length);
            qualifiedName[length] = 0;
            super = &getInterface(qualifiedName);
            inheritedMethodCount += super->getMethodCount();
        }
        interface->setInheritedMethodCount(inheritedMethodCount);
    }

    // TODO: Constructor registration should be automated, too.
    registerConstructor(Alarm::iid(),
                        reinterpret_cast<Object* (*)()>(Alarm::getConstructor), reinterpret_cast<void (*)(Object*)>(Alarm::setConstructor));
    registerConstructor(Cache::iid(),
                        reinterpret_cast<Object* (*)()>(Cache::getConstructor), reinterpret_cast<void (*)(Object*)>(Cache::setConstructor));
    registerConstructor(Monitor::iid(),
                        reinterpret_cast<Object* (*)()>(Monitor::getConstructor), reinterpret_cast<void (*)(Object*)>(Monitor::setConstructor));
    registerConstructor(PageSet::iid(),
                        reinterpret_cast<Object* (*)()>(PageSet::getConstructor), reinterpret_cast<void (*)(Object*)>(PageSet::setConstructor));
    registerConstructor(Process::iid(),
                        reinterpret_cast<Object* (*)()>(Process::getConstructor), reinterpret_cast<void (*)(Object*)>(Process::setConstructor));
    registerConstructor(FatFileSystem::iid(),
                        reinterpret_cast<Object* (*)()>(FatFileSystem::getConstructor), reinterpret_cast<void (*)(Object*)>(FatFileSystem::setConstructor));
    registerConstructor(Iso9660FileSystem::iid(),
                        reinterpret_cast<Object* (*)()>(Iso9660FileSystem::getConstructor), reinterpret_cast<void (*)(Object*)>(Iso9660FileSystem::setConstructor));
}

namespace
{
    InterfaceDataStore interfaceStore __attribute__((init_priority(1000)));    // Before System
}

Reflect::Interface& getInterface(const char* iid)
{
    return interfaceStore.getInterface(iid);
}

Object* getConstructor(const char* iid)
{
    return interfaceStore.getConstructor(iid);
}

void registerConstructor(const char* iid, Object* (*getter)(), void (*setter)(Object*))
{
    return interfaceStore.registerConstructor(iid, getter, setter);
}

void registerConstructor(const char* iid, Object* constructor)
{
    return interfaceStore.registerConstructor(iid, constructor);
}

const char* getUniqueIdentifier(const char* iid)
{
    return interfaceStore.getUniqueIdentifier(iid);
}

}  // namespace es
