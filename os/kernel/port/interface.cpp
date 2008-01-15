/*
 * Copyright 2008 Google Inc.
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
#include <es.h>
#include <es/hashtable.h>
#include "interfaceStore.h"

unsigned char* InterfaceStore::defaultInterfaceInfo[] =
{
    IAlarmInfo,
    ICacheInfo,
    ICallbackInfo,
    IClassFactoryInfo,
    IClassStoreInfo,
    IFileInfo,
    IInterfaceInfo,
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
    IFileSystemInfo,
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

void InterfaceStore::
registerInterface(Reflect::Module& module)
{
    for (int i = 0; i < module.getInterfaceCount(); ++i)
    {
        Reflect::Interface interface(module.getInterface(i));

        SpinLock::Synchronized method(spinLock);
        hashtable.add(interface.getIid(), interface);
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
    for (int i = 0;
         i < sizeof defaultInterfaceInfo / sizeof defaultInterfaceInfo[0];
         ++i)
    {
        Reflect r(defaultInterfaceInfo[i]);
        Reflect::Module global(r.getGlobalModule());
        registerInterface(global);
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
remove(const Guid& riid)
{
    SpinLock::Synchronized method(spinLock);

    hashtable.remove(riid);
    // XXX release buffer
}

void* InterfaceStore::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == IInterfaceStore::iid())
    {
        objectPtr = static_cast<IInterfaceStore*>(this);
    }
    else if (riid == IInterface::iid())
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
addRef(void)
{
    return ref.addRef();
}

unsigned int InterfaceStore::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
