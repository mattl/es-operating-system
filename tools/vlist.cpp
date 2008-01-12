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
#include <stdio.h>
#include <stdlib.h>
#include <es.h>
#include <es/clsid.h>
#include <es/dateTime.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/base/IClassStore.h>
#include <es/base/IStream.h>
#include <es/base/IFile.h>
#include <es/device/IFileSystem.h>
#include <es/naming/IContext.h>
#include <es/handle.h>
#include "vdisk.h"

void list(Handle<IContext> root, const char* filename)
{
    filename = filename ? filename : "";
    Handle<IIterator> iter = root->list(filename);
    while (iter->hasNext())
    {
        char name[1024];
        Handle<IBinding> binding(iter->next());
        binding->getName(name, sizeof name);
        Handle<IFile> file(binding->getObject());
        long long size;
        size = file->getSize();
        long long t;
        file->getLastWriteTime(t);
        DateTime d(t);
        esReport("%8lld %2d/%2d/%2d %02d:%02d:%02d %s\t\n",
                 size,
                 d.getYear(),
                 d.getMonth(),
                 d.getDay(),
                 d.getHour(),
                 d.getMinute(),
                 d.getSecond(),
                 name);
    }
}

int main(int argc, char* argv[])
{
    IInterface* ns = 0;
    esInit(&ns);
    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterFatFileSystemClass(classStore);

    if (argc < 2)
    {
        esReport("usage: %s disk_image [directory]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Handle<IStream> disk = new VDisk(static_cast<char*>(argv[1]));
    Handle<IFileSystem> fatFileSystem;
    esCreateInstance(CLSID_FatFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&fatFileSystem));
    fatFileSystem->mount(disk);
    fatFileSystem->checkDisk(false);
    esReport("\n");

    {
        Handle<IContext> root;

        fatFileSystem->getRoot(reinterpret_cast<IContext**>(&root));
        list(root, argv[2]);
    }

    long long freeSpace;
    long long totalSpace;
    fatFileSystem->getFreeSpace(freeSpace);
    fatFileSystem->getTotalSpace(totalSpace);
    esReport("\nFree space %lld, Total space %lld\n", freeSpace, totalSpace);

    fatFileSystem->dismount();
    fatFileSystem = 0;
}
