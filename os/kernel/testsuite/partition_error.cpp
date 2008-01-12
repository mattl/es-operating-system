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
#include "vdisk.h"
#include "partition.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

IDiskManagement::Geometry VDiskGeometry;

static void SetData(u8* buf, long long size)
{
    buf[size-1] = 0;
    while (--size)
    {
        *buf++ = 'a' + size % 26;
    }
}

void PrintPartitions(IContext* context)
{
    char name[16];
    IDiskManagement::Partition params;
    esReport("boot type offset   size\n");
    Handle<IIterator> iter = context->list("");
    Handle<IBinding> binding;
    while ((binding = iter->next()))
    {
        Handle<IDiskManagement> diskManagement = binding->getObject();
        TEST(diskManagement);
        diskManagement->getLayout(&params);
        TEST(0 < binding->getName(name, sizeof(name)));
        esReport("%02x   %02x   %08llx %08llx %s\n",
                 params.bootIndicator,
                 params.partitionType,
                 params.startingOffset,
                 params.partitionLength,
                 name);

        Handle<IStream> stream = context->lookup(name);
        TEST(stream);
        long long size;
        size = stream->getSize();
        TEST(params.partitionLength == size);
        TEST(params.bootIndicator == 0x00 || params.bootIndicator == 0x80);
        TEST(params.partitionType != 0x00);
    }
    esReport("\n");
}

void CheckGeometry(Handle<IDiskManagement> dm)
{
    IDiskManagement::Geometry geometry;
    dm->getGeometry(&geometry); // throw exception when error occurs.
#ifdef VERBOSE
    esReport("Geometry\n");
    esReport("  heads           %u\n", geometry.heads);
    esReport("  cylinders       %u\n", geometry.cylinders);
    esReport("  sectorsPerTrack %u\n", geometry.sectorsPerTrack);
    esReport("  bytesPerSector  %u\n", geometry.bytesPerSector);
    esReport("  diskSize        %llu\n", geometry.diskSize);
#endif // VERBOSE
    TEST(geometry.heads == VDiskGeometry.heads);
    TEST(geometry.cylinders == VDiskGeometry.cylinders);
    TEST(geometry.sectorsPerTrack == VDiskGeometry.sectorsPerTrack);
    TEST(geometry.bytesPerSector == VDiskGeometry.bytesPerSector);
    TEST(geometry.diskSize == VDiskGeometry.diskSize);
}

void CreatePartition(IContext* context, const char* name, long long& size, u8 type)
{
    esReport("Create: %s (size %lld, type 0x%02x)\n", name ,size, type);

    Handle<IBinding> binding;
    binding = context->bind(name, 0);
    TEST(binding);

    Handle<IStream> stream = context->lookup(name);
    TEST(stream);
    Handle<IStream> object = binding->getObject();
    TEST(stream == object);

    CheckGeometry(stream);

    Handle<IDiskManagement> diskManagement = stream;
    IDiskManagement::Partition params;
    diskManagement->getLayout(&params);

    stream->setSize(size);
    size = stream->getSize();

    // set the partition type.
    diskManagement->getLayout(&params);
    params.partitionType = type;

    diskManagement->setLayout(&params);
    diskManagement->getLayout(&params);
    TEST(params.partitionType == type);
}

void TestSetSize(IContext* context)
{
    Handle<IStream> stream;
    stream = context->lookup("partition0");
    long long size0;
    size0 = stream->getSize();

    stream = context->lookup("partition1");
    Handle<IDiskManagement> dm = stream;
    IDiskManagement::Geometry geometry;
    dm->getGeometry(&geometry);

    long long size;
    size = stream->getSize();

    // Set too large size.
    long long largeSize;
    largeSize = geometry.diskSize - size0;
    largeSize += 1024 * 1024LL;
    stream->setSize(largeSize);
    long long newSize;
    newSize = stream->getSize();

    // check the size is not changed.
    TEST(size == newSize);
}

void TestSetSize2(IContext* context)
{
    Handle<IStream> stream;
    stream = context->lookup("extended");
    long long size;
    size = stream->getSize();

    Handle<IDiskManagement> dm = stream;
    IDiskManagement::Geometry geometry;
    dm->getGeometry(&geometry);

    long long largeSize;
    largeSize = size + 1024 * 1024LL;
    stream->setSize(largeSize);
    long long newSize;
    newSize = stream->getSize();

    // check the size is not changed.
    TEST(size == newSize);

    stream = context->lookup("logical1");
    size = stream->getSize();

    largeSize = size + 1024 * 1024LL;
    stream->setSize(largeSize);
    newSize = stream->getSize();

    // check the size is not changed.
    TEST(size == newSize);
}

void TestWrite(IContext* context)
{
    Handle<IStream> stream;
    stream = context->lookup("partition1");
    long long size;
    size = stream->getSize();

    long long largeSize = size + 1024 * 1024;

    // write data.
    long long ret;
    u8* writeBuf = new u8[largeSize];
    SetData(writeBuf, size);
    ret = stream->write(writeBuf, size, 0);
    TEST(ret == size);
    stream->flush();

    // read data.
    u8* readBuf = new u8[size];
    memset(readBuf, 0, size);
    ret = stream->read(readBuf, size, 0);
    TEST(ret == size);
    TEST(memcmp(readBuf, writeBuf, size) == 0);

    // check errors.
    ret = stream->write(writeBuf, largeSize, 0);
    TEST(ret < 0);
    ret = stream->read(readBuf, largeSize, 0);
    TEST(ret < 0);

    delete [] writeBuf;
    delete [] readBuf;
}

void TestBind(IContext* context)
{
    Handle<IBinding> binding;
    binding = context->bind("logical4", 0);
    TEST(!binding);

    // partition3 cannot be created,
    // when the extended patrition exisits.
    binding = context->bind("partition3", 0);
    TEST(!binding);

    // check a partition which contains partitions cannot be removed.
    TEST(context->unbind("extended") < 0);
    TEST(context->unbind("logical0") < 0);
    TEST(context->unbind("logical1") < 0);

    TEST(context->unbind("logical256") < 0);
    TEST(context->unbind("partition4") < 0);
    TEST(context->unbind("extended1") < 0);

}

void RemoveLogicalAndExtendedPartitions(IContext* context)
{
    TEST(context->unbind("logical2") == 0);
    TEST(context->unbind("logical1") == 0);
    TEST(context->unbind("logical0") == 0);
    TEST(context->unbind("extended") == 0);
}

void Test(IContext* context)
{
    Handle<IBinding> binding;
    Handle<IStream> stream;

    // Create partition0.
    long long size = 16 * 1024 * 1024LL;
    CreatePartition(context, "partition0", size, 0x04);

    // Create partition1.
    binding = context->bind("partition1", 0);
    TEST(binding);
    stream = context->lookup("partition1");

    TestSetSize(context);
    TestWrite(context);

    size = 2 * 1024 * 1024LL;
    CreatePartition(context, "partition1", size, 0x04);

    size = 8 * 1024 * 1024LL;
    CreatePartition(context, "extended", size, 0x05);

    size = 1024 * 1024LL;
    CreatePartition(context, "logical0", size, 0x01);

    size = 1024 * 1024LL;
    CreatePartition(context, "logical1", size, 0x01);

    size = 1024 * 1024LL;
    CreatePartition(context, "logical2", size, 0x01);

    size = 1024 * 1024LL;
    CreatePartition(context, "partition2", size, 0x01);

    TestBind(context);
    TestSetSize2(context);

    RemoveLogicalAndExtendedPartitions(context);

    size = 3 * 1024 * 1024LL;
    CreatePartition(context, "partition3", size, 0x04);
}

void Init(IContext* context)
{
    // remove all partitions.
    esReport("Remove all partitions.\n");
    context->unbind("partition0");
    context->unbind("partition1");
    context->unbind("partition2");
    context->unbind("partition3");

    char name[16];
    int id = 0;
    sprintf(name, "logical%u", id);

    Handle<IStream> stream;
    Handle<IBinding> binding;
    while (stream = context->lookup(name))
    {
        ++id;
        sprintf(name, "logical%u", id);
    }

    while (id)
    {
        --id;
        sprintf(name, "logical%u", id);
        context->unbind(name);
    }

    context->unbind("extended0");

    // check
    stream = context->lookup("partition0");
    TEST(!stream);
    stream = context->lookup("partition1");
    TEST(!stream);
    stream = context->lookup("partition2");
    TEST(!stream);
    stream = context->lookup("partition3");
    TEST(!stream);
    stream = context->lookup("extended0");
    TEST(!stream);
}

int main(int argc, char* argv[])
{
    IInterface* ns = 0;
    esInit(&ns);

    Handle<IStream> disk = new VDisk(static_cast<char*>("fat16_32MB.img"));
    Handle<IDiskManagement> dm = disk;
    dm->getGeometry(&VDiskGeometry);

    Handle<IPartition> partition = new PartitionContext();
    TEST(partition);

    // mount
    TEST(partition->mount(disk) == 0);
    {
        Handle<IContext> context = partition;
        TEST(context);

        Init(context);

        Test(context);

        // show results.
        esReport("\nPartition Entry Table\n");
        PrintPartitions(context);
    }

    // unmount
    TEST(partition->unmount() == 0);

    esReport("done.\n\n");
}
