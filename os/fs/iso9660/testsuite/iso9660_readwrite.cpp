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
#include "iso9660Stream.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

static long PacketRead(IStream* stream, long long size, long numPacket, bool useOffset)
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

void test(Handle<IContext> root)
{
    long long           size = 0;

    Handle<IContext>    dir = root->lookup("data");
    TEST(dir);

    Handle<IFile>       file = dir->lookup("image");
    TEST(dir);

    size = file->getSize();
    u8* buf = new u8[size];
    Handle<IStream> stream = file->getStream();
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
    IInterface* ns = 0;
    esInit(&ns);
    Handle<IContext> nameSpace(ns);

    Handle<IClassStore> classStore(nameSpace->lookup("class"));
    esRegisterIsoFileSystemClass(classStore);

#ifdef __es__
    Handle<IStream> disk = nameSpace->lookup("device/ata/channel1/device0");
#else
    IStream* disk = new VDisk(static_cast<char*>("isotest.iso"));
#endif
    long long diskSize;
    diskSize = disk->getSize();
    esReport("diskSize: %lld\n", diskSize);

    Handle<IFileSystem> isoFileSystem;
    esCreateInstance(CLSID_IsoFileSystem, IID_IFileSystem,
                     reinterpret_cast<void**>(&isoFileSystem));
    TEST(isoFileSystem);
    isoFileSystem->mount(disk);
    {
        Handle<IContext> root;

        isoFileSystem->getRoot(reinterpret_cast<IContext**>(&root));
        TEST(root);
        test(root);
    }
    isoFileSystem->dismount();

    esReport("done.\n");
}
