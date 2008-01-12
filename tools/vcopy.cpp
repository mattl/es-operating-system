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

#include <new>
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

void copy(Handle<IContext> root, char* filename)
{
    FILE* f = fopen(filename, "rb");
    if (f == 0)
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

    Handle<IFile> file(root->bind(p, 0));
    if (!file)
    {
        file = root->lookup(p);
    }
    Handle<IStream> stream(file->getStream());
    stream->setSize(0);
    for (;;)
    {
        long n = fread(sec, 1, 512, f);
        if (n < 0)
        {
            exit(EXIT_FAILURE);
        }
        if (n == 0)
        {
            return;
        }
        do {
            long len = stream->write(sec, n);
            if (len <= 0)
            {
                exit(EXIT_FAILURE);
            }
            n -= len;
        } while (0 < n);
    }
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
    esCreateInstance(CLSID_FatFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&fatFileSystem));
    fatFileSystem->mount(disk);

    long long freeSpace;
    long long totalSpace;
    fatFileSystem->getFreeSpace(freeSpace);
    fatFileSystem->getTotalSpace(totalSpace);
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);

    {
        Handle<IContext> root;

        fatFileSystem->getRoot(reinterpret_cast<IContext**>(&root));
        copy(root, argv[2]);
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;
}
