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

#include "interfaceStore.h"
#include <es/interfaceData.h>

struct ConstructorAccessors
{
    const char* iid;
    Object* (*constructorGetter)();                 // for statically created data
    void (*constructorSetter)(Object* constructor); // for statically created data
};

ConstructorAccessors defaultConstructorInfo[] =
{
    {
        es::Alarm::iid(),
        reinterpret_cast<Object* (*)()>(es::Alarm::getConstructor),
        reinterpret_cast<void (*)(Object*)>(es::Alarm::setConstructor)
    },
    {
        es::Cache::iid(),
        reinterpret_cast<Object* (*)()>(es::Cache::getConstructor),
        reinterpret_cast<void (*)(Object*)>(es::Cache::setConstructor)
    },
    {
        es::Monitor::iid(),
        reinterpret_cast<Object* (*)()>(es::Monitor::getConstructor),
        reinterpret_cast<void (*)(Object*)>(es::Monitor::setConstructor)
    },
    {
        es::PageSet::iid(),
        reinterpret_cast<Object* (*)()>(es::PageSet::getConstructor),
        reinterpret_cast<void (*)(Object*)>(es::PageSet::setConstructor)
    },
    {
        es::Process::iid(),
        reinterpret_cast<Object* (*)()>(es::Process::getConstructor),
        reinterpret_cast<void (*)(Object*)>(es::Process::setConstructor)
    },
    {
        es::FatFileSystem::iid(),
        reinterpret_cast<Object* (*)()>(es::FatFileSystem::getConstructor),
        reinterpret_cast<void (*)(Object*)>(es::FatFileSystem::setConstructor)
    },
    {
        es::Iso9660FileSystem::iid(),
        reinterpret_cast<Object* (*)()>(es::Iso9660FileSystem::getConstructor),
        reinterpret_cast<void (*)(Object*)>(es::Iso9660FileSystem::setConstructor)
    },
};

void InterfaceStore::
registerConstructor(const char* iid, Object* (*getter)(), void (*setter)(Object*))
{
    MetaData* data = &hashtable.get(iid);
    data->constructorGetter = getter;
    data->constructorSetter = setter;
}

void InterfaceStore::
updateInheritedMethodCount(const char* iid)
{
    unsigned inheritedMethodCount = 0;
    Reflect::Interface* interface = &getInterface(iid);
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

InterfaceStore::
InterfaceStore()
{
    for (es::InterfaceData* data = es::interfaceData; data->iid; ++data)
    {
        MetaData metaData(data->info(), data->iid());
        hashtable.add(data->iid(), metaData);
    }

    // Update inheritedMethodCount of each interface data.
    for (es::InterfaceData* data = es::interfaceData; data->iid; ++data)
    {
        updateInheritedMethodCount(data->iid());
    }

    for (int i = 0;
         i < sizeof defaultConstructorInfo / sizeof defaultConstructorInfo[0];
         ++i)
    {
        ConstructorAccessors* accessors = &defaultConstructorInfo[i];
        registerConstructor(accessors->iid, accessors->constructorGetter, accessors->constructorSetter);
    }
}

InterfaceStore::
~InterfaceStore()
{
}

void InterfaceStore::
add(const char* iid, const char* info)
{
    char* kiid = strdup(iid);
    char* kinfo = strdup(info);
    if (!kiid || !kinfo)
    {
        free(kiid);
        free(kinfo);
        return;
    }
    MetaData metaData(kinfo, kiid);
    hashtable.add(kiid, metaData);
    updateInheritedMethodCount(kiid);
}

void InterfaceStore::
remove(const char* iid)
{
    // TODO: not implemented
}

Object* InterfaceStore::
queryInterface(const char* iid)
{
    Object* objectPtr;
    if (strcmp(iid, es::InterfaceStore::iid()) == 0)
    {
        objectPtr = static_cast<es::InterfaceStore*>(this);
    }
    else if (strcmp(iid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::InterfaceStore*>(this);
    }
    else
    {
        return 0;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int InterfaceStore::
addRef()
{
    return ref.addRef();
}

unsigned int InterfaceStore::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
