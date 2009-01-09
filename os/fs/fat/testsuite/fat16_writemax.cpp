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

static void SetData(u8* buf, long long size)
{
    buf[size-1] = 0;
    while (--size)
    {
        *buf++ = 'a' + size % 26;
    }
}

static long TestFileSystem(Handle<IContext> root)
{
    int freeCount = 5083;
    int secPerClus = 2;
    int bytsPerSec = 512;

    Handle<IFile> file = root->bind("test1.txt", 0);
    TEST(file);

    Handle<IStream> stream = file->getStream();

    // The disk fills up.
    long size = freeCount * secPerClus * bytsPerSec;
    u8* buf = new u8[size];

    SetData(buf, size);
    long ret = stream->write(buf, size);
    TEST(ret == size);

    file = 0;
    file = root->bind("test2.txt", 0);
    TEST(file);
    file = 0;
    file = root->bind("test3.txt", 0);
    TEST(file);
    file = 0;

    root->unbind("test1.txt");
    delete [] buf;

    esReport("done.\n");

    // The disk overflows.
    file = root->bind("test4.txt", 0);
    buf = new u8[size+1];

    SetData(buf, size+1);
    stream = file->getStream();

    try
    {
        ret = stream->write(buf, size+1);
        TEST(ret == 0);
    }
    catch (SystemException<ENOSPC>& e)
    {

    }

    delete [] buf;

    root->unbind("test1.txt");
    root->unbind("test2.txt");
    root->unbind("test3.txt");
    root->unbind("test4.txt");

    return 0;
}

int main(void)
{
    IInterface* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<IContext> nameSpace(ns);

#ifdef __es__
    Handle<IStream> disk = nameSpace->lookup("device/ata/channel0/device0");
#else
    Handle<IStream> disk = new VDisk(static_cast<char*>("fat16_5MB.img"));
#endif
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<IFileSystem> fatFileSystem;
    long long freeSpace;
    long long totalSpace;

    fatFileSystem = IFatFileSystem::createInstance();
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    freeSpace = fatFileSystem->getFreeSpace();
    totalSpace = fatFileSystem->getTotalSpace();
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    {
        Handle<IContext> root;

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

    fatFileSystem = IFatFileSystem::createInstance();
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
