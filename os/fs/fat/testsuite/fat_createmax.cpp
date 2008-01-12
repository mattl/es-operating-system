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
    int i;
    int rootEntCnt = 224;
    int freeCount = 2847;
    // create directories in the root.
    char dirName[256];
    for (i = 0; i < rootEntCnt; ++i)
    {
        sprintf(dirName, "dir%d", i);
#ifdef VERBOSE
        esReport("create \"%s\"\n", dirName);
#endif // VERBOSE

        Handle<IFile> dir = root->createSubcontext(dirName);
        TEST(dir);

        TEST(dir->isDirectory());
        TEST(dir->canRead());
        TEST(dir->canWrite());
        TEST(!dir->isHidden());
        TEST(!dir->isFile());

        char created[512];
        dir->getName(created, sizeof(created));
        TEST (strcmp(created, dirName) == 0);
        --freeCount;
    }

    try
    {
        sprintf(dirName, "dir%d", rootEntCnt);
        Handle<IFile> dir = root->createSubcontext(dirName);
        TEST(!dir);
    }
    catch (SystemException<EINVAL>& e)
    {
        // Invalid argument
    }

    // create files in a directory.
    sprintf(dirName, "dir0");
    Handle<IContext> dir = root->lookup(dirName);
    TEST(dir);
    Handle<IFile> dir0 = dir;
    TEST(dir0);
    long ret;
    long long dirSize;
    dirSize = dir0->getSize();
    char fileName[256];
    for (i = 0; i < freeCount - (dirSize/512 - 1); ++i)
    {
        sprintf(fileName, "file%00d", i);
#ifdef VERBOSE
        esReport("create dir0/%s (dir0: %lld)\n", fileName, dirSize);
#endif // VERBOSE

        Handle<IFile> file = dir->bind(fileName, 0);
        TEST(file);
        Handle<IStream> stream = file->getStream();
        ret = stream->write("test", 5);
        TEST(ret == 5);

        dirSize = dir0->getSize();
    }

    try
    {
        sprintf(fileName, "file%00d", freeCount);
        Handle<IFile> file = dir->bind(fileName, 0);
        TEST(file);
        Handle<IStream> stream = file->getStream();
        ret = stream->write("test", 5);
        TEST(ret != 5);
    }
    catch (SystemException<ENOSPC>& e)
    {

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
        long ret = TestFileSystem(root);
        TEST (ret == 0);
        freeSpace = fatFileSystem->getFreeSpace();
        totalSpace = fatFileSystem->getTotalSpace();
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        esReport("\nChecking the file system...\n");
        TEST(fatFileSystem->checkDisk(false));
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
}
