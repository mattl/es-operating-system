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

#include <string.h>
#include <es/hashtable.h>
#include <es/reflect.h>
#include <es/base/IInterface.h>
#include <es/base/IAlarm.h>
#include <es/base/ICache.h>
#include <es/base/IMonitor.h>
#include <es/base/IPageSet.h>
#include <es/base/IProcess.h>
#include <es/device/IFatFileSystem.h>
#include <es/device/IIso9660FileSystem.h>
#include <es/device/IPartition.h>

//
// Reflection data of the default interface set
//

extern unsigned char IAlarmInfo[];
extern unsigned char ICacheInfo[];
extern unsigned char ICallbackInfo[];
extern unsigned char IFileInfo[];
extern unsigned char IInterfaceInfo[];
extern unsigned char IInterfaceStoreInfo[];
extern unsigned char IMonitorInfo[];
extern unsigned char IPageableInfo[];
extern unsigned char IPageSetInfo[];
extern unsigned char IProcessInfo[];
extern unsigned char IRuntimeInfo[];
extern unsigned char ISelectableInfo[];
extern unsigned char IServiceInfo[];
extern unsigned char IStreamInfo[];
extern unsigned char IThreadInfo[];

extern unsigned char IAudioFormatInfo[];
extern unsigned char IBeepInfo[];
extern unsigned char ICursorInfo[];
extern unsigned char IDeviceInfo[];
extern unsigned char IDiskManagementInfo[];
extern unsigned char IDmacInfo[];
extern unsigned char IFatFileSystemInfo[];
extern unsigned char IFileSystemInfo[];
extern unsigned char IIso9660FileSystemInfo[];
extern unsigned char IPicInfo[];
extern unsigned char IRemovableMediaInfo[];
extern unsigned char IRtcInfo[];
extern unsigned char IPartitionInfo[];

extern unsigned char IBindingInfo[];
extern unsigned char IContextInfo[];

extern unsigned char IInternetAddressInfo[];
extern unsigned char IInternetConfigInfo[];
extern unsigned char IResolverInfo[];
extern unsigned char ISocketInfo[];

extern unsigned char IIteratorInfo[];
extern unsigned char ISetInfo[];

extern unsigned char ICanvasRenderingContext2DInfo[];

extern unsigned char IOrderedMapInfo[];

namespace es
{

class InterfaceStore
{
    struct InterfaceData
    {
        Reflect::Interface meta;
        IInterface* (*constructorGetter)();                 // for statically created data
        void (*constructorSetter)(IInterface* constructor); // for statically created data
        IInterface* constructor;                            // for dynamically created data

        InterfaceData() :
            constructorGetter(0),
            constructorSetter(0),
            constructor(0)
        {
        }

        InterfaceData(Reflect::Interface interface) :
            meta(interface),
            constructorGetter(0),
            constructorSetter(0),
            constructor(0)
        {
        }

        IInterface* getConstructor() const
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

        void setConstructor(IInterface* constructor)
        {
            if (constructorSetter)
            {
                constructorSetter(constructor);
                return;
            }
            this->constructor = constructor;
        }
    };

    Hashtable<const char*, InterfaceData, Hash<const char*>, Reflect::CompareName> hashtable;

    void registerInterface(Reflect::Module& module);

public:
    InterfaceStore(int capacity = 128);

    Reflect::Interface& getInterface(const char* iid)
    {
        return hashtable.get(iid).meta;
    }

    IInterface* getConstructor(const char* iid)
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

    void registerConstructor(const char* iid, IInterface* (*getter)(), void (*setter)(IInterface*))
    {
        try
        {
            InterfaceData* data = &hashtable.get(iid);
            data->constructorGetter = getter;
            data->constructorSetter = setter;
        }
        catch (...)
        {
        }
    }

    void registerConstructor(const char* iid, IInterface* constructor)
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
            return hashtable.get(iid).meta.getFullyQualifiedName();
        }
        catch (...)
        {
            return 0;
        }
    }
};

unsigned char* defaultInterfaceInfo[] =
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

    IOrderedMapInfo,
};

size_t defaultInterfaceCount = sizeof defaultInterfaceInfo / sizeof defaultInterfaceInfo[0];

void InterfaceStore::
registerInterface(Reflect::Module& module)
{
    for (int i = 0; i < module.getInterfaceCount(); ++i)
    {
        InterfaceData data(module.getInterface(i));
        hashtable.add(data.meta.getFullyQualifiedName(), data);
    }

    for (int i = 0; i < module.getModuleCount(); ++i)
    {
        Reflect::Module m(module.getModule(i));
        registerInterface(m);
    }
}

InterfaceStore::
InterfaceStore(int capacity) :
    hashtable(capacity)
{
    for (int i = 0; i < defaultInterfaceCount; ++i)
    {
        Reflect r(defaultInterfaceInfo[i]);
        Reflect::Module global(r.getGlobalModule());
        registerInterface(global);
    }

    registerConstructor(IAlarm::iid(),
                        reinterpret_cast<IInterface* (*)()>(IAlarm::getConstructor), reinterpret_cast<void (*)(IInterface*)>(IAlarm::setConstructor));
    registerConstructor(ICache::iid(),
                        reinterpret_cast<IInterface* (*)()>(ICache::getConstructor), reinterpret_cast<void (*)(IInterface*)>(ICache::setConstructor));
    registerConstructor(IMonitor::iid(),
                        reinterpret_cast<IInterface* (*)()>(IMonitor::getConstructor), reinterpret_cast<void (*)(IInterface*)>(IMonitor::setConstructor));
    registerConstructor(IPageSet::iid(),
                        reinterpret_cast<IInterface* (*)()>(IPageSet::getConstructor), reinterpret_cast<void (*)(IInterface*)>(IPageSet::setConstructor));
    registerConstructor(IProcess::iid(),
                        reinterpret_cast<IInterface* (*)()>(IProcess::getConstructor), reinterpret_cast<void (*)(IInterface*)>(IProcess::setConstructor));
    registerConstructor(IFatFileSystem::iid(),
                        reinterpret_cast<IInterface* (*)()>(IFatFileSystem::getConstructor), reinterpret_cast<void (*)(IInterface*)>(IFatFileSystem::setConstructor));
    registerConstructor(IIso9660FileSystem::iid(),
                        reinterpret_cast<IInterface* (*)()>(IIso9660FileSystem::getConstructor), reinterpret_cast<void (*)(IInterface*)>(IIso9660FileSystem::setConstructor));
    registerConstructor(IPartition::iid(),
                        reinterpret_cast<IInterface* (*)()>(IPartition::getConstructor), reinterpret_cast<void (*)(IInterface*)>(IPartition::setConstructor));
}

namespace
{
    InterfaceStore interfaceStore __attribute__((init_priority(1000)));    // Before System
}

Reflect::Interface& getInterface(const char* iid)
{
    return interfaceStore.getInterface(iid);
}

IInterface* getConstructor(const char* iid)
{
    return interfaceStore.getConstructor(iid);
}

void registerConstructor(const char* iid, IInterface* (*getter)(), void (*setter)(IInterface*))
{
    return interfaceStore.registerConstructor(iid, getter, setter);
}

void registerConstructor(const char* iid, IInterface* constructor)
{
    return interfaceStore.registerConstructor(iid, constructor);
}

const char* getUniqueIdentifier(const char* iid)
{
    return interfaceStore.getUniqueIdentifier(iid);
}

}  // namespace es