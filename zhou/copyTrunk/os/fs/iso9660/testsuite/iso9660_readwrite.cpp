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
#include "iso9660Stream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static long PacketRead(es::Stream* stream, long long size, long numPacket, bool useOffset)
{
#ifdef VERBOSE
    esReport("size %lld bytes, numPacket %d, useOffset %d\n", size, numPacket, useOffset);
#endif // VERBOSE

    u8* buf = new u8[size];
    u8* data = new u8[size];

    long long n;
    n = stream->read(data, size, 0);
    TEST(n == size);

    long long packetSize;
    if (size % numPacket == 0)
    {
        packetSize = size / numPacket;
    }
    else
    {
        packetSize = size / (numPacket - 1);
    }

    stream->setPosition(0);

    long i;
    long long offset = 0;
    long long pos;
    if (useOffset)
    {
        // long read(void* dst, long count, long long offset);
        for (i = 0; i < numPacket; ++i)
        {
            if (size - offset < packetSize)
            {
                packetSize = size - offset;
            }
            pos = stream->getPosition();
            TEST(pos == 0);
            n = stream->read(buf + offset, packetSize, offset);
            TEST(n == packetSize);
            offset += packetSize;
        }
    }
    else
    {
        // long read(void* dst, long count);
        for (i = 0; i < numPacket; ++i)
        {
            if (size - offset < packetSize)
            {
                packetSize = size - offset;
            }
            stream->setPosition(offset);
            pos = stream->getPosition();
            TEST(pos == offset);
            n = stream->read(buf + offset, packetSize);
            TEST(n == packetSize);
            offset += packetSize;
        }
    }

    TEST(memcmp(buf, data, size) == 0);
    delete [] buf;
    delete [] data;

    return 0;
}

void test(Handle<es::Context> root)
{
    long long           size = 0;

    Handle<es::Context>    dir = root->lookup("data");
    TEST(dir);

    Handle<es::File>       file = dir->lookup("image");
        TEST(file);

    size = file->getSize();
    u8* buf = new u8[size];
    Handle<es::Stream> stream = file->getStream();
    long long ret = stream->read(buf, size);
    TEST(ret == size);

    try
    {
        ret = stream->write(buf, size);
    }
    catch (SystemException<EACCES>& e)
    {
        // Permission denied
    }

    try
    {
        ret = stream->write(buf, size - 1024, 1024);
    }
    catch (SystemException<EACCES>& e)
    {
        // Permission denied
    }

    delete [] buf;

    TEST(PacketRead(stream, size, 2, 0) == 0);
    TEST(PacketRead(stream, size, 2, 1) == 0);
    TEST(PacketRead(stream, size, 10, 0) == 0);
    TEST(PacketRead(stream, size, 10, 1) == 0);
    TEST(PacketRead(stream, size, size/1024, 0) == 0);
    TEST(PacketRead(stream, size, size/1024, 1) == 0);
}

int main(int argc, char* argv[])
{
    Object* ns = 0;
    esInit(&ns);
    Iso9660FileSystem::initializeConstructor();
    Handle<es::Context> nameSpace(ns);

#ifdef __es__
    Handle<es::Stream> disk = nameSpace->lookup("device/ata/channel1/device0");
#else
    es::Stream* disk = new VDisk(static_cast<char*>("isotest.iso"));
#endif
    TEST(disk);
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);
    TEST(0 < diskSize);

    Handle<es::FileSystem> isoFileSystem;
    isoFileSystem = es::Iso9660FileSystem::createInstance();
    TEST(isoFileSystem);
    isoFileSystem->mount(disk);
    {
        Handle<es::Context> root;

        root = isoFileSystem->getRoot();
        TEST(root);
        test(root);
    }
    isoFileSystem->dismount();

    esReport("done.\n");
}
