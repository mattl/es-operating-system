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
#include <es/base/IInterface.h>
#include <es/base/IAlarm.h>
#include <es/base/ICache.h>
#include <es/base/IMonitor.h>
#include <es/base/IPageSet.h>
#include <es/base/IProcess.h>
#include <es/device/IFatFileSystem.h>
#include <es/device/IIso9660FileSystem.h>
#include <es/device/IPartition.h>

using namespace es;

unsigned char* InterfaceStore::defaultInterfaceInfo[] =
{
    // Base classes first
    IInterfaceInfo,

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
    IDiskManagementInfo,
    IDmacInfo,
    IFatFileSystemInfo,
    IFileSystemInfo,
    IIso9660FileSystemInfo,
    IPicInfo,
    IRemovableMediaInfo,
    IRtcInfo,
    IPartitionInfo,

    IBindingInfo,
    IContextInfo,

    IInternetAddressInfo,
    IInternetConfigInfo,
    IResolverInfo,
    ISocketInfo,

    IIteratorInfo,
    ISetInfo,

    ICanvasRenderingContext2DInfo,
};

struct ConstructorAccessors
{
    const char* iid;
    IInterface* (*constructorGetter)();                 // for statically created data
    void (*constructorSetter)(IInterface* constructor); // for statically created data
};

ConstructorAccessors defaultConstructorInfo[] =
{
    {
        IAlarm::iid(),
        reinterpret_cast<IInterface* (*)()>(IAlarm::getConstructor),
        reinterpret_cast<void (*)(IInterface*)>(IAlarm::setConstructor)
    },
    {
        ICache::iid(),
        reinterpret_cast<IInterface* (*)()>(ICache::getConstructor),
        reinterpret_cast<void (*)(IInterface*)>(ICache::setConstructor)
    },
    {
        IMonitor::iid(),
        reinterpret_cast<IInterface* (*)()>(IMonitor::getConstructor),
        reinterpret_cast<void (*)(IInterface*)>(IMonitor::setConstructor)
    },
    {
        IPageSet::iid(),
        reinterpret_cast<IInterface* (*)()>(IPageSet::getConstructor),
        reinterpret_cast<void (*)(IInterface*)>(IPageSet::setConstructor)
    },
    {
        IProcess::iid(),
        reinterpret_cast<IInterface* (*)()>(IProcess::getConstructor),
        reinterpret_cast<void (*)(IInterface*)>(IProcess::setConstructor)
    },
    {
        IFatFileSystem::iid(),
        reinterpret_cast<IInterface* (*)()>(IFatFileSystem::getConstructor),
        reinterpret_cast<void (*)(IInterface*)>(IFatFileSystem::setConstructor)
    },
    {
        IIso9660FileSystem::iid(),
        reinterpret_cast<IInterface* (*)()>(IIso9660FileSystem::getConstructor),
        reinterpret_cast<void (*)(IInterface*)>(IIso9660FileSystem::setConstructor)
    },
    {
        IPartition::iid(),
        reinterpret_cast<IInterface* (*)()>(IPartition::getConstructor),
        reinterpret_cast<void (*)(IInterface*)>(IPartition::setConstructor)
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
registerConstructor(const char* iid, IInterface* (*getter)(), void (*setter)(IInterface*))
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

void* InterfaceStore::
queryInterface(const char* riid)
{
    void* objectPtr;
    if (strcmp(riid, IInterfaceStore::iid()) == 0)
    {
        objectPtr = static_cast<IInterfaceStore*>(this);
    }
    else if (strcmp(riid, IInterface::iid()) == 0)
    {
        objectPtr = static_cast<IInterfaceStore*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
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
