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

#include <stdlib.h>
#include <es.h>
#include <es/reflect.h>
#include "process.h"

static unsigned char* DefaultInterfaceInfo[] =
{
    IAlarmInfo,
    ICacheInfo,
    ICallbackInfo,
    IClassFactoryInfo,
    IClassStoreInfo,
    IFileInfo,
    IInterfaceInfo,
    IMonitorInfo,
    IPageableInfo,
    IPageSetInfo,
    IProcessInfo,
    IRuntimeInfo,
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

    IIteratorInfo,
    ISetInfo,
};

static Reflect::Interface DefulatInterfaceTable[256];

// for qsort()
static int compareInterface(const void* a, const void* b)
{
    const Guid* g1 = static_cast<const Reflect::Interface*>(a)->getIid();
    const Guid* g2 = static_cast<const Reflect::Interface*>(b)->getIid();

    for (int i = 0; i < 4; ++i)
    {
        u32 e1 = ((u32*) g1)[i];
        u32 e2 = ((u32*) g2)[i];
        if (e1 < e2)
        {
            return -1;
        }
        if (e2 < e1)
        {
            return 1;
        }
    }
    return 0;
}

// for bsearch()
static int compareIid(const void* a, const void* b)
{
    const Guid* g1 = static_cast<const Guid*>(a);
    const Guid* g2 = static_cast<const Reflect::Interface*>(b)->getIid();

    for (int i = 0; i < 4; ++i)
    {
        u32 e1 = ((u32*) g1)[i];
        u32 e2 = ((u32*) g2)[i];
        if (e1 < e2)
        {
            return -1;
        }
        if (e2 < e1)
        {
            return 1;
        }
    }
    return 0;
}

static int interfaceCount(0);

void initInterfaceInfo()
{
    for (int i = 0; i < sizeof DefaultInterfaceInfo / sizeof DefaultInterfaceInfo[0]; ++i)
    {
        Reflect r(DefaultInterfaceInfo[i]);
        for (int j = 0; j < r.getInterfaceCount(); ++j)
        {
            if (r.getInterface(j).getType().isImported())
            {
                continue;
            }
            DefulatInterfaceTable[interfaceCount] = r.getInterface(j);
            ++interfaceCount;
        }
    }

    qsort(DefulatInterfaceTable, interfaceCount, sizeof(Reflect::Interface), compareInterface);
#ifdef VERBOSE
    for (int i = 0; i < interfaceCount; ++i)
    {
        esReport("interface %s\n", DefulatInterfaceTable[i].getName());
    }
#endif
}

Reflect::Interface* getInterface(const Guid* iid)
{
    return static_cast<Reflect::Interface*>(
        bsearch(static_cast<const void*>(iid),
                static_cast<const void*>(DefulatInterfaceTable),
                interfaceCount,
                sizeof(Reflect::Interface),
                compareIid));
}
