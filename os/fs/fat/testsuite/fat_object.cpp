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
#include <stdlib.h>
#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include "vdisk.h"
#include "fatStream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static long TestFileSystem(Handle<IContext> root)
{
    const char* dirList[] =
    {
        "dir1",
        "dir2",
        "dir3",
        "dir4",
        "dir5",
        "dir6",
        "dir7",
        "dir8",
        "dir9"
    };

    int i;
    int len;
    for (i = 0; i < sizeof(dirList)/sizeof(dirList[0]); ++i)
    {
        len = strlen(dirList[i]);
        Handle<IFile> file = root->lookup(dirList[i]);
        if (!file)
        {
            file = root->createSubcontext(dirList[i]);
            TEST(file->isDirectory());
            TEST(file->canRead());
            TEST(file->canWrite());
            TEST(!file->isHidden());
            TEST(!file->isFile());
        }

        if (file)
        {
            char created[512];
            file->getName(created, sizeof(created));
            if (strcmp(created, dirList[i]) != 0)
            {
                esReport("failed. \"%s\" is created.\n", created);
                return -1;
            }
        }
        else
        {
            esReport("failed.\n");
            return -1;
        }
    }

    return 0;
}

int main(void)
{
    IInterface* ns = 0;
    esInit(&ns);
    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterFatFileSystemClass(classStore);

#ifdef __es__
    Handle<IStream> disk = nameSpace->lookup("device/floppy");
#else
    Handle<IStream> disk = new VDisk(static_cast<char*>("2hd.img"));
#endif
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<IFileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    fatFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    {
        Handle<IContext> root;
        root = fatFileSystem->getRoot();

        Handle<IBinding> binding = root;
        TEST(binding);

        Handle<IInterface> interface = binding->getObject();
        TEST(interface);

        Handle<IContext> object = interface;
        TEST(object);
        TEST(object == root);

        long ret = TestFileSystem(object);
        TEST(ret == 0);

        // setObject() must return an exception.
        try
        {
            binding->setObject(interface);
            TEST(false);
        }
        catch (Exception& error)
        {
        }
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;

    fatFileSystem = reinterpret_cast<IFileSystem*>(
        esCreateInstance(CLSID_FatFileSystem, IFileSystem::iid()));
    fatFileSystem->mount(disk);
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    esReport("\nChecking the file system...\n");
    TEST(fatFileSystem->checkDisk(false));
    fatFileSystem->dismount();
    fatFileSystem = 0;

    esReport("done.\n\n");
    return 0;
}
