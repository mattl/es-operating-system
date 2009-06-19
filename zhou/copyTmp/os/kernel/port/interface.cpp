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
#include <string.h>
#include <es.h>
#include <es/hashtable.h>
#include <es/object.h>
#include <es/base/IAlarm.h>
#include <es/base/ICache.h>
#include <es/base/IMonitor.h>
#include <es/base/IPageSet.h>
#include <es/base/IProcess.h>
#include <es/device/IFatFileSystem.h>
#include <es/device/IIso9660FileSystem.h>

unsigned char* InterfaceStore::defaultInterfaceInfo[] =
{
    // Base classes first
    objectInfo,

    IAlarmInfo,
    ICacheInfo,
    ICallbackInfo,
    IFileInfo,
    IInterfaceStoreInfo,
    IMonitorInfo,
    IPageableInfo,
    IPageSetInfo,
    IProcessInfo,
    IRuntimeInfo,
    ISelectableInfo,
    IServiceInfo,
    IStreamInfo,
    IThreadInfo,

    IAudioFormatInfo,
    IBeepInfo,
    ICursorInfo,
    IDeviceInfo,
    IDiskInfo,
    IDmacInfo,
    IFatFileSystemInfo,
    IFileSystemInfo,
    IIso9660FileSystemInfo,
    IPicInfo,
    IRemovableMediaInfo,
    IRtcInfo,

    IBindingInfo,
    IContextInfo,

    IInternetAddressInfo,
    IInternetConfigInfo,
    IResolverInfo,
    ISocketInfo,

    IIteratorInfo,
    ISetInfo,

    cssInfo,
    cssomviewInfo,
    domInfo,
    eventsInfo,
    html5Info,
    lsInfo,
    rangesInfo,
    smilInfo,
    stylesheetsInfo,
    svgInfo,
    traversalInfo,
    validationInfo,
    viewsInfo,
};

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
registerInterface(Reflect::Module& module)
{
    for (int i = 0; i < module.getInterfaceCount(); ++i)
    {
        Reflect::Interface interface(module.getInterface(i));

        SpinLock::Synchronized method(spinLock);
        hashtable.add(interface.getFullyQualifiedName(), interface);
    }

    for (int i = 0; i < module.getModuleCount(); ++i)
    {
        Reflect::Module m(module.getModule(i));
        registerInterface(m);
    }
}

void InterfaceStore::
registerConstructor(const char* iid, Object* (*getter)(), void (*setter)(Object*))
{
    InterfaceData* data = &hashtable.get(iid);
    data->constructorGetter = getter;
    data->constructorSetter = setter;
}

InterfaceStore::
InterfaceStore(int capacity) :
    hashtable(capacity)
{
    for (int i = 0;
         i < sizeof defaultInterfaceInfo / sizeof defaultInterfaceInfo[0];
         ++i)
    {
        Reflect r(defaultInterfaceInfo[i]);
        Reflect::Module global(r.getGlobalModule());
        registerInterface(global);
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
add(const void* data, int length)
{
    void* buffer = new u8[length];
    memmove(buffer, data, length);
    Reflect r(buffer);
    Reflect::Module global(r.getGlobalModule());
    registerInterface(global);
}

void InterfaceStore::
remove(const char* riid)
{
    SpinLock::Synchronized method(spinLock);

    hashtable.remove(riid);
    // XXX release buffer
}

Object* InterfaceStore::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::InterfaceStore::iid()) == 0)
    {
        objectPtr = static_cast<es::InterfaceStore*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::InterfaceStore*>(this);
    }
    else
    {
        return NULL;
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
