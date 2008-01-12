/*
 * Copyright (c) 2006
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

#ifndef NINTENDO_ES_KERNEL_INTERFACE_STORE_H_INCLUDED
#define NINTENDO_ES_KERNEL_INTERFACE_STORE_H_INCLUDED

#include <es.h>
#include <es/hashtable.h>
#include <es/ref.h>
#include <es/reflect.h>
#include <es/base/IInterfaceStore.h>
#include "thread.h"

class InterfaceStore : public IInterfaceStore
{
    static unsigned char* defaultInterfaceInfo[];

    SpinLock    spinLock;
    Ref         ref;
    Hashtable<Guid, Reflect::Interface>
                hashtable;

public:
    InterfaceStore(int capacity = 128);
    ~InterfaceStore();

    Reflect::Interface& getInterface(const Guid& iid)
    {
        SpinLock::Synchronized method(spinLock);

        return hashtable.get(iid);
    }

    // IInterfaceStore
    void add(const void* data, int length);
    void remove(const Guid& riid);

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);
};

//
// Reflection data of the default interface set
//

extern unsigned char IAlarmInfo[];
extern unsigned char ICacheInfo[];
extern unsigned char ICallbackInfo[];
extern unsigned char IClassFactoryInfo[];
extern unsigned char IClassStoreInfo[];
extern unsigned char IFileInfo[];
extern unsigned char IInterfaceInfo[];
extern unsigned char IInterfaceStoreInfo[];
extern unsigned char IMonitorInfo[];
extern unsigned char IPageableInfo[];
extern unsigned char IPageSetInfo[];
extern unsigned char IProcessInfo[];
extern unsigned char IRuntimeInfo[];
extern unsigned char IStreamInfo[];
extern unsigned char IThreadInfo[];

extern unsigned char IAudioFormatInfo[];
extern unsigned char IBeepInfo[];
extern unsigned char ICursorInfo[];
extern unsigned char IDeviceInfo[];
extern unsigned char IDiskManagementInfo[];
extern unsigned char IDmacInfo[];
extern unsigned char IFileSystemInfo[];
extern unsigned char IPicInfo[];
extern unsigned char IRemovableMediaInfo[];
extern unsigned char IRtcInfo[];
extern unsigned char IPartitionInfo[];

extern unsigned char IBindingInfo[];
extern unsigned char IContextInfo[];

extern unsigned char IIteratorInfo[];
extern unsigned char ISetInfo[];

Reflect::Interface& getInterface(const Guid& iid);

#endif // NINTENDO_ES_KERNEL_INTERFACE_STORE_H_INCLUDED
