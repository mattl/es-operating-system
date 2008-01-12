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

void test(Handle<IContext> root)
{
    Handle<IFile>       file1;

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
    Handle<IStream> stream1 = file1->getStream();
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

    esCreateInstance(CLSID_FatFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&fatFileSystem));
    fatFileSystem->mount(disk);
    fatFileSystem->format();
    fatFileSystem->getFreeSpace(freeSpace);
    fatFileSystem->getTotalSpace(totalSpace);
    esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
    {
        Handle<IContext> root;

        fatFileSystem->getRoot(reinterpret_cast<IContext**>(&root));
        test(root);
        fatFileSystem->getFreeSpace(freeSpace);
        fatFileSystem->getTotalSpace(totalSpace);
        esReport("Free space %lld, Total space %lld\n", freeSpace, totalSpace);
        esReport("\nChecking the file system...\n");
        TEST(fatFileSystem->checkDisk(false));
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
