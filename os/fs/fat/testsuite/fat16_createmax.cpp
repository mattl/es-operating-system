/*
 * Copyright 2011 Esrille Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include "vdisk.h"
#include "fatStream.h"

#define VERBOSE 1

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static long TestFileSystem(Handle<es::Context> root)
{
    int i;
    int rootEntCnt = 512;
    int freeCount  = 5083;
    int secPerClus = 2;
    int bytsPerSec = 512;
    // create directories in the root.
    char dirName[256];
    for (i = 0; i < rootEntCnt; ++i)
    {
        sprintf(dirName, "dir%d", i);
#ifdef VERBOSE
        esReport("create \"%s\"\n", dirName);
#endif // VERBOSE

        Handle<es::File> dir = root->createSubcontext(dirName);
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
        Handle<es::File> dir = root->createSubcontext(dirName);
        TEST(!dir);
    }
    catch (SystemException<EINVAL>& e)
    {
        // Invalid argument
    }

    // create files in a directory.
    sprintf(dirName, "dir0");
    Handle<es::Context> dir = root->lookup(dirName);
    TEST(dir);
    Handle<es::File> dir0 = dir;
    TEST(dir0);
    long ret;
    long long dirSize;
    dirSize = dir0->getSize();
    char fileName[256];
    for (i = 0; i < freeCount - (dirSize/(bytsPerSec*secPerClus) - 1); ++i)
    {
        sprintf(fileName, "file%00d", i);
#ifdef VERBOSE
        esReport("create dir0/%s (dir0: %lld, %lld)\n", fileName, dirSize,
            freeCount - (dirSize/(bytsPerSec*secPerClus) - 1) - i);
#endif // VERBOSE

        Handle<es::File> file = dir->bind(fileName, 0);
        TEST(file);
        Handle<es::Stream> stream = file->getStream();
        ret = stream->write("test", 5);
        TEST(ret == 5);

        dirSize = dir0->getSize();
    }

    try
    {
        sprintf(fileName, "file%00d", freeCount);
        Handle<es::File> file = dir->bind(fileName, 0);
        TEST(file);
        Handle<es::Stream> stream = file->getStream();
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
    Object* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<es::Context> nameSpace(ns);

#ifdef __es__
    Handle<es::Stream> disk = nameSpace->lookup("device/ata/channel0/device0");
#else
    Handle<es::Stream> disk = new VDisk(static_cast<char*>("fat16_5MB.img"));
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
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    {
        Handle<es::Context> root;

        root = fatFileSystem->getRoot();
        TestFileSystem(root);
        freeSpace = fatFileSystem->getFreeSpace();
        totalSpace = fatFileSystem->getTotalSpace();
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        esReport("\nChecking the file system...\n");
        TEST(fatFileSystem->checkDisk(false));
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
