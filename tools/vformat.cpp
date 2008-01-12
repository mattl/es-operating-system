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

#include <new>
#include <errno.h>
#include <stdlib.h>
#include <es.h>
#include <es/clsid.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/base/IClassStore.h>
#include <es/base/IStream.h>
#include <es/device/IFileSystem.h>
#include <es/naming/IContext.h>
#include <es/handle.h>
#include "vdisk.h"

int main(int argc, char* argv[])
{
    IInterface* ns = 0;
    esInit(&ns);
    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterFatFileSystemClass(classStore);

    if (argc < 2)
    {
        esReport("usage: %s disk_image [cylinders] [heads] [sectorsPerTrack]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Handle<IStream> disk;
    if (argc < 5)
    {
        disk = new VDisk(static_cast<char*>(argv[1]));
    }
    else
    {
        unsigned int cylinders = atoi(argv[2]);
        unsigned int heads = atoi(argv[3]);
        unsigned int sectorsPerTrack = atoi(argv[4]);
        disk = new VDisk(static_cast<char*>(argv[1]), cylinders, heads, sectorsPerTrack);
    }
    Handle<IFileSystem> fatFileSystem;
    fatFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
    try
    {
        fatFileSystem->mount(disk);
    }
    catch (SystemException<EINVAL>& e)
    {
    }
    fatFileSystem->format();
    fatFileSystem->dismount();
    fatFileSystem = 0;
}
