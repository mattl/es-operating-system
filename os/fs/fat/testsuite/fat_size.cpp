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

static long TestReadWrite(es::Stream* stream, long long& size)
{
    u8* writeBuf;
    u8* readBuf;
    long ret = 0;

    writeBuf = new u8[size];
    readBuf = new u8[size];
    memset(readBuf, 0, size);

    SetData(writeBuf, size);

    ret = stream->write(writeBuf, size);
    TEST(ret == size);
    stream->flush();

    stream->setPosition(0);
    ret = stream->read(readBuf, size);
    TEST(ret == size);

    TEST (memcmp(writeBuf, readBuf, size) == 0);

    delete [] writeBuf;
    delete [] readBuf;
    return ret;
}


static long TestFileSystem(Handle<es::Context> root)
{
    Handle<es::File>       file;

    const char* filename = "test.txt";

    file = root->bind(filename, 0);
    Handle<es::Stream> stream = file->getStream();

    long long sizeWritten = 6 * 1024LL;
    long ret = TestReadWrite(stream, sizeWritten);
    if (ret < 0)
    {
        return -1;
    }

    long long size;
    size = stream->getSize();
    TEST(size == sizeWritten);

    long long newSize = sizeWritten * 2;
    stream->setSize(newSize);
    size = stream->getSize();
    TEST(newSize == size);

    newSize = sizeWritten/2;
    stream->setSize(newSize);
    size = stream->getSize();
    TEST(newSize == size);

    return 0;
}

int main(void)
{
    es::Interface* ns = 0;
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
