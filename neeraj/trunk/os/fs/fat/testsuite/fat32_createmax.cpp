/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

static long TestFileSystem(Handle<es::Context> root)
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

    Handle<es::Context> context;
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

        Handle<es::File> dir = context;
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
            Handle<es::File> file = context->bind(fileName, 0);
            TEST(file);
            --freeCount;
            ++totalFile;
            Handle<es::Stream> stream = file->getStream();
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
        Handle<es::File> file = context->bind(fileName, 0);
        TEST(file);
        Handle<es::Stream> stream = file->getStream();
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
    Object* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<es::Context> nameSpace(ns);

#ifdef __es__
    Handle<es::Stream> disk = nameSpace->lookup("device/ata/channel0/device0");
#else
    Handle<es::Stream> disk = new VDisk(static_cast<char*>("fat32.img"));
#endif
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<es::FileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    fatFileSystem = es::FatFileSystem::createInstance();
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();

    try
    {
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        Handle<es::Context> root;

        root = fatFileSystem->getRoot();
        long ret = TestFileSystem(root);
        TEST (ret == 0);
        freeSpace = fatFileSystem->getFreeSpace();
        totalSpace = fatFileSystem->getTotalSpace();
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

    fatFileSystem = es::FatFileSystem::createInstance();
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
