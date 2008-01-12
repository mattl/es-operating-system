/*
 * Copyright (c) 2006, 2007
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
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
