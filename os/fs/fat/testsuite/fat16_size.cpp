/*
 * Copyright 2008 Google Inc.
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

static long TestReadWrite(IStream* stream, long long& size)
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


static long TestFileSystem(Handle<IContext>  root)
{
    Handle<IFile>       file;

    const char* filename = "test.txt";

    file = root->bind(filename, 0);
    Handle<IStream> stream = file->getStream();

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
    IInterface* ns = 0;
    esInit(&ns);
    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterFatFileSystemClass(classStore);

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
        TestFileSystem(root);
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
