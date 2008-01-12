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
#include <es/reflect.h>
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
};

InterfaceStore::
InterfaceStore(int capacity) :
    hashtable(capacity)
{
    for (int i = 0;
         i < sizeof defaultInterfaceInfo / sizeof defaultInterfaceInfo[0];
         ++i)
    {
        Reflect r(defaultInterfaceInfo[i]);
        for (int j = 0; j < r.getInterfaceCount(); ++j)
        {
            if (r.getInterface(j).getType().isImported())
            {
                continue;
            }
            hashtable.add(r.getInterface(j).getIid(), r.getInterface(j));
        }
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
    for (int i = 0; i < r.getInterfaceCount(); ++i)
    {
        if (r.getInterface(i).getType().isImported())
        {
            continue;
        }
        {
            SpinLock::Synchronized method(spinLock);

            hashtable.add(r.getInterface(i).getIid(), r.getInterface(i));
        }
    }
}

void InterfaceStore::
remove(const Guid& riid)
{
    SpinLock::Synchronized method(spinLock);

    hashtable.remove(riid);
}

bool InterfaceStore::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IInterfaceStore)
    {
        *objectPtr = static_cast<IInterfaceStore*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IInterfaceStore*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
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
