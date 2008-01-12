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
    int freeCount  = 130811;
    int secPerClus = 8;
    int bytsPerSec = 512;
    int clusterSize = bytsPerSec*secPerClus;
    char dirName[256];
    long ret;
    char fileName[256];
    int numDir = 0;
    int offset = 0;
    int needed;

    int totalFile = 0;
    int totalDirCluster = 0;

    Handle<IContext> context;
    // create files in a directory.
    while (0 < freeCount)
    {
        sprintf(dirName, "dir%d", numDir);
        context = root->createSubcontext(dirName);
        TEST(context);
        --freeCount;
        ++totalDirCluster;
        if (freeCount == 0)
        {
            break;
        }

        Handle<IFile> dir = context;
        TEST(dir);
        long long dirSize;
        long long newSize;
        dirSize = dir->getSize();

        for (i = 1; i < 2048; ++i)
        {
            if ((i + 1) % (clusterSize/32) == 0)
            {
                needed = 2;
            }
            else
            {
                needed = 1;
            }
            if (freeCount < needed)
            {
                break;
            }

            sprintf(fileName, "%05d", i);
            Handle<IFile> file = context->bind(fileName, 0);
            TEST(file);
            --freeCount;
            ++totalFile;
            Handle<IStream> stream = file->getStream();
            ret = stream->write("test", 5);
#ifdef VERBOSE
            esReport("%s/%s (%d) fc: %d\n", dirName, fileName, ret, freeCount);
#endif // VERBOSE
            TEST(ret == 5);

            // An additional cluster is assigned.
            newSize = dir->getSize();
            if (dirSize < newSize)
            {
                ++totalDirCluster;
                --freeCount;
                dirSize = newSize;
            }
        }
#ifdef VERBOSE
        esReport("Total files %d, Total clusters for dirs %d\n",
            totalFile, totalDirCluster);
#endif // VERBOSE
        ++numDir;
    }

    try
    {
        sprintf(fileName, "fileXXX");
        esReport("write %s\n", fileName);
        Handle<IFile> file = context->bind(fileName, 0);
        TEST(file);
        Handle<IStream> stream = file->getStream();
        ret = stream->write("test", 5);
        esReport("ret %d\n", ret);
        TEST(ret != 5);
    }
    catch (SystemException<ENOSPC>& e)
    {
        esReport("Exception!\n");
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
    Handle<IStream> disk = nameSpace->lookup("device/ata/channel0/device0");
#else
    Handle<IStream> disk = new VDisk(static_cast<char*>("fat32.img"));
#endif
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<IFileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    esCreateInstance(CLSID_FatFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&fatFileSystem));
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    fatFileSystem->getFreeSpace(freeSpace);
    fatFileSystem->getTotalSpace(totalSpace);

    try
    {
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        Handle<IContext> root;

        fatFileSystem->getRoot(reinterpret_cast<IContext**>(&root));
        long ret = TestFileSystem(root);
        TEST (ret == 0);
        fatFileSystem->getFreeSpace(freeSpace);
        fatFileSystem->getTotalSpace(totalSpace);
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        esReport("\nChecking the file system...\n");
        TEST(fatFileSystem->checkDisk(false));
    }
    catch (...)
    {
        esReport("Caught an exception.\n");
        TEST(false);
    }
    fatFileSystem->dismount();
    fatFileSystem = 0;

    esCreateInstance(CLSID_FatFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&fatFileSystem));
    fatFileSystem->mount(disk);
    fatFileSystem->getFreeSpace(freeSpace);
    fatFileSystem->getTotalSpace(totalSpace);
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    esReport("\nChecking the file system...\n");
    TEST(fatFileSystem->checkDisk(false));
    fatFileSystem->dismount();
    fatFileSystem = 0;

    esReport("done.\n\n");
}
