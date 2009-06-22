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
#include <es/hashtable.h>
#include <es/reflect.h>
#include <es/object.h>
#include <es/base/IAlarm.h>
#include <es/base/ICache.h>
#include <es/base/IMonitor.h>
#include <es/base/IPageSet.h>
#include <es/base/IProcess.h>
#include <es/device/IFatFileSystem.h>
#include <es/device/IIso9660FileSystem.h>

//
// Reflection data of the default interface set
//

extern unsigned char objectInfo[];

extern unsigned char IAlarmInfo[];
extern unsigned char ICacheInfo[];
extern unsigned char ICallbackInfo[];
extern unsigned char IFileInfo[];
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
extern unsigned char IDiskInfo[];
extern unsigned char IDmacInfo[];
extern unsigned char IFatFileSystemInfo[];
extern unsigned char IFileSystemInfo[];
extern unsigned char IIso9660FileSystemInfo[];
extern unsigned char IPicInfo[];
extern unsigned char IRemovableMediaInfo[];
extern unsigned char IRtcInfo[];

extern unsigned char IBindingInfo[];
extern unsigned char IContextInfo[];

extern unsigned char IInternetAddressInfo[];
extern unsigned char IInternetConfigInfo[];
extern unsigned char IResolverInfo[];
extern unsigned char ISocketInfo[];

extern unsigned char IIteratorInfo[];
extern unsigned char ISetInfo[];

extern unsigned char IOrderedMapInfo[];

extern unsigned char cssInfo[];
extern unsigned char cssomviewInfo[];
extern unsigned char domInfo[];
extern unsigned char eventsInfo[];
extern unsigned char html5Info[];
extern unsigned char lsInfo[];
extern unsigned char rangesInfo[];
extern unsigned char smilInfo[];
extern unsigned char stylesheetsInfo[];
extern unsigned char svgInfo[];
extern unsigned char traversalInfo[];
extern unsigned char validationInfo[];
extern unsigned char viewsInfo[];

namespace es
{

class InterfaceStore
{
    struct InterfaceData
    {
        Reflect::Interface meta;
        Object* (*constructorGetter)();                 // for statically created data
        void (*constructorSetter)(Object* constructor); // for statically created data
        Object* constructor;                            // for dynamically created data

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

    Hashtable<const char*, InterfaceData, Hash<const char*>, Reflect::CompareName> hashtable;

    void registerInterface(Reflect::Module& module);

public:
    InterfaceStore(int capacity = 1024);

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
            InterfaceData* data = &hashtable.get(iid);
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

    IOrderedMapInfo,

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
    InterfaceStore interfaceStore __attribute__((init_priority(1000)));    // Before System
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