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
#include <es/hashtable.h>
#include <es/ref.h>
#include <es/reflect.h>
#include <es/base/IInterfaceStore.h>
#include "thread.h"

class InterfaceStore : public es::InterfaceStore
{
    static unsigned char* defaultInterfaceInfo[];

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
    Hashtable<const char*, InterfaceData, Hash<const char*>, Reflect::CompareName> hashtable;

    void registerInterface(Reflect::Module& module);
    void registerConstructor(const char* iid, Object* (*getter)(), void (*setter)(Object*));

public:
    InterfaceStore(int capacity = 1024);
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
            return hashtable.get(iid).meta.getFullyQualifiedName();
        }
        catch (...)
        {
            return 0;
        }
    }

    // IInterfaceStore
    void add(const void* data, int length);
    void remove(const char* riid);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

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
    Reflect::Interface& getInterface(const char* iid);
    Object* getConstructor(const char* iid);
    const char* getUniqueIdentifier(const char* iid);
}  // namespace es

#endif // NINTENDO_ES_KERNEL_INTERFACE_STORE_H_INCLUDED
