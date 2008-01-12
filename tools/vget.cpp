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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <es.h>
#include <es/clsid.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/base/IClassStore.h>
#include <es/base/IStream.h>
#include <es/base/IFile.h>
#include <es/device/IFileSystem.h>
#include <es/naming/IContext.h>
#include <es/handle.h>
#include "vdisk.h"

u8 sec[512];

void get(Handle<IContext> root, char* filename)
{
    Handle<IFile> file(root->lookup(filename));
    if (!file)
    {
        esReport("Could not open %s\n", filename);
        exit(EXIT_FAILURE);
    }

    const char* p = filename;
    p += strlen(p);
    while (filename < --p)
    {
        if (strchr(":/\\", *p))
        {
            ++p;
            break;
        }
    }
    FILE* f = fopen(p, "wb");
    if (f == 0)
    {
        esReport("Could not create %s\n", p);
        exit(EXIT_FAILURE);
    }

    Handle<IStream> stream(file->getStream());
    long size = stream->getSize();
    while (0 < size)
    {
        long len = stream->read(sec, 512);
        if (len <= 0)
        {
            exit(EXIT_FAILURE);
        }
        fwrite(sec, len, 1, f);
        size -= len;
    }
    fclose(f);
}

int main(int argc, char* argv[])
{
    IInterface* ns = 0;
    esInit(&ns);
    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterFatFileSystemClass(classStore);

    if (argc < 3)
    {
        esReport("usage: %s disk_image file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Handle<IStream> disk = new VDisk(static_cast<char*>(argv[1]));
    Handle<IFileSystem> fatFileSystem;
    fatFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
    fatFileSystem->mount(disk);

    long long freeSpace;
    long long totalSpace;
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);

    {
        Handle<IContext> root;

        root = fatFileSystem->getRoot();
        get(root, argv[2]);
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;
}
