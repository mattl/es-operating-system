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
#include <stdlib.h>
#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include "vdisk.h"
#include "fatStream.h"

#define PAGE_SIZE   4096

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static void setData(u8* buf, long size, u8 shift)
{
    for (long i = 0; i < size; ++i)
    {
        *buf++ = 'A' + (i + shift) % 26;
    }
}

void test(Handle<es::Context> root)
{
    Handle<es::File>       file1;

    unsigned long maxPage = 14;
#ifdef VERBOSE
    esReport("maxPage: %lu\n", maxPage);
#endif // VERBOSE

    long size1 = maxPage * PAGE_SIZE * 2;
    long size2 = maxPage * PAGE_SIZE * 2;
    u8* buf1 = new u8[size1];
    setData(buf1, size1, 1);
    u8* buf2 = new u8[size1];
    setData(buf2, size1, 2);

    file1 = root->bind("a", 0);
    TEST(file1);
    Handle<es::Stream> stream1 = file1->getStream();
    TEST(stream1);

    long rc;

    // Associate all pages to cache 1.
    rc = stream1->write(buf1, size1, 0);
    TEST(rc == size1);
    stream1->flush();

    // Verify cache1 content
    rc = stream1->read(buf2, size1, 0);
    TEST(rc == size1);
#ifdef VERBOSE
    for (long offset = 0; offset < size1; offset += PAGE_SIZE)
    {
        esReport("%08x: %02x %02x\n", offset, buf1[offset], buf2[offset]);
    }
#endif // VERBOSE
    TEST(memcmp(buf1, buf2, rc) == 0);
}

int main(void)
{
    Object* ns = 0;
    esInit(&ns);
    FatFileSystem::initializeConstructor();
    Handle<es::Context> nameSpace(ns);

#ifdef __es__
    Handle<es::Stream> disk = nameSpace->lookup("device/floppy");
#else
    Handle<es::Stream> disk = new VDisk(static_cast<char*>("2hd.img"));
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
        test(root);
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
