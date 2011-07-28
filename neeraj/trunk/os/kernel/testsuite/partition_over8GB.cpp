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
#include "vdisk.h"
#include "partition.h"
#include <es/endian.h>

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

using namespace LittleEndian;

es::DiskManagement::Geometry VDiskGeometry;

static void SetData(u8* buf, long long size)
{
    buf[size-1] = 0;
    while (--size)
    {
        *buf++ = 'a' + size % 26;
    }
}

void PrintPartitions(es::Context* context)
{
    char name[16];
    es::DiskManagement::Partition params;
    esReport("boot type offset   size\n");
    Handle<es::Iterator> iter = context->list("");
    Handle<es::Binding> binding;
    while ((binding = iter->next()))
    {
        Handle<es::DiskManagement> diskManagement = binding->getObject();
        TEST(diskManagement);
        diskManagement->getLayout(&params);
        TEST(0 < binding->getName(name, sizeof(name)));
        esReport("%02x   %02x   %08llx %08llx %s\n",
                 params.bootIndicator,
                 params.partitionType,
                 params.startingOffset,
                 params.partitionLength,
                 name);

        Handle<es::Stream> stream = context->lookup(name);
        TEST(stream);
        long long size;
        size = stream->getSize();
        TEST(params.partitionLength == size);
        TEST(params.bootIndicator == 0x00 || params.bootIndicator == 0x80);
        TEST(params.partitionType != 0x00);
    }
    esReport("\n");
}

void CheckGeometry(Handle<es::DiskManagement> dm)
{
    es::DiskManagement::Geometry geometry;
    TEST(dm->getGeometry(&geometry) == 0);
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

void CreatePartition(es::Context* context, const char* name, long long& size, u8 type)
{
    esReport("Create: %s (size %lld, type 0x%02x)\n", name ,size, type);

    Handle<es::Binding> binding;
    binding = context->bind(name, 0);
    TEST(binding);

    Handle<es::Stream> stream = context->lookup(name);
    TEST(stream);
    Handle<es::Stream> object = binding->getObject();
    TEST(stream == object);

    CheckGeometry(stream);

    Handle<es::DiskManagement> dm = stream;
    es::DiskManagement::Partition params;
    dm->getLayout(&params);
    es::DiskManagement::Geometry geometry;
    TEST(dm->getGeometry(&geometry) == 0);

    stream->setSize(size);
    size = stream->getSize();

    // set the partition type.
    TEST(dm->getLayout(&params) == 0);

    es::DiskManagement::Partition newParams = params;
    params.partitionType = type;

    TEST(dm->setLayout(&params) == 0);
    TEST(dm->getLayout(&newParams) == 0);

    if (params.partitionType == 0x05 ||
        params.partitionType == 0x0f)
    {
        if (1023 * 255 * 63LL * (long long) geometry.bytesPerSector <=
            (params.startingOffset + params.partitionLength))
        {
            TEST(newParams.partitionType == 0x0f);
        }
        else
        {
            TEST(newParams.partitionType == 0x05);
        }
    }
    else
    {
        TEST(newParams.partitionType == type);
    }

}

void TestReadWrite(es::Stream* stream, long long size, long long offset)
{
    // write data.
    long long ret;
    u8* writeBuf = new u8[size];
    SetData(writeBuf, size);
    ret = stream->write(writeBuf, size, offset);
    TEST(ret == size);
    stream->flush();

    // read data.
    u8* readBuf = new u8[size];
    memset(readBuf, 0, size);
    ret = stream->read(readBuf, size, offset);
    TEST(ret == size);

    // check.
    TEST(memcmp(readBuf, writeBuf, size) == 0);

    // cleanup.
    memset(writeBuf, 0, size);
    ret = stream->write(writeBuf, size, offset);

    delete [] writeBuf;
    delete [] readBuf;
}

void Test(es::Context* context)
{
    Handle<es::Binding> binding;
    Handle<es::Stream> stream;

    long long size = 8 * 1024 * 1024 * 1024LL;
    CreatePartition(context, "partition0", size, 0x0C);

    size = 16 * 1024 * 1024LL;
    CreatePartition(context, "partition1", size, 0x0C);

    stream = context->lookup("partition1");
    size = 1024 * 1024;
    long long offset = 8 * 1024 * 1024;

    TestReadWrite(stream, size, offset);
}

void TestExtended(es::Context* context)
{
    Handle<es::Binding> binding;
    Handle<es::Stream> stream;

    long long size = 8 * 1024 * 1024 * 1024LL - 1024 * 1024LL;
    CreatePartition(context, "partition0", size, 0x0B);

    size = 64 * 1024 * 1024LL;
    CreatePartition(context, "extended", size, 0x05);

    stream = context->lookup("extended");
    Handle<es::DiskManagement> dm = stream;
    TEST(dm);
    es::DiskManagement::Partition params;
    dm->getLayout(&params);

    TEST(params.partitionType == 0x0f);

    size = 32 * 1024 * 1024LL;
    CreatePartition(context, "logical0", size, 0x0C);

    size = 32 * 1024 * 1024LL;
    CreatePartition(context, "logical1", size, 0x0C);
}

void Init(es::Context* context)
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

    Handle<es::Stream> stream;
    Handle<es::Binding> binding;
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

enum
{
    MBR_PartitionTable = 446,       // Offset to the partition table
    MBR_EntrySize = 16,             // Partition table entry size
    MBR_Signature = 510,            // Offset to the boot sector signature (0xaa55)

    MBR_BootIndicator = 0,
    MBR_StartingCHS = 1,
    MBR_SystemIndicator = 4,
    MBR_EndingCHS = 5,
    MBR_StartingSector = 8,         // in LBA
    MBR_TotalSectors = 12           // in LBA
};

void CheckMbr(es::Stream* disk)
{
    es::DiskManagement::Geometry& geometry = VDiskGeometry;

    // Read MBR
    u8 mbr[geometry.bytesPerSector];
    disk->read(mbr, geometry.bytesPerSector, 0);
    TEST(word(mbr + MBR_Signature) == 0xaa55);

    u8* entry = &mbr[MBR_PartitionTable];
    int i;
    unsigned startingCHS;
    unsigned endingCHS;
    unsigned startingSector;
    unsigned totalSector;

    u8* extended = 0;
    for (i = 0; i < 4; ++i, entry += MBR_EntrySize)
    {
#ifdef VERBOSE
        esReport("%02x %06x %02x %06x %08x %08x\n",
                 byte(entry + MBR_BootIndicator),
                 dword(entry + MBR_StartingCHS) & 0xffffff,
                 byte(entry + MBR_SystemIndicator),
                 dword(entry + MBR_EndingCHS) & 0xffffff,
                 dword(entry + MBR_StartingSector),
                 dword(entry + MBR_TotalSectors));
#endif // VERBOSE
        startingCHS = dword(entry + MBR_StartingCHS) & 0xffffff;
        endingCHS = dword(entry + MBR_EndingCHS) & 0xffffff;
        startingSector = dword(entry + MBR_StartingSector);
        totalSector = dword(entry + MBR_TotalSectors);
        if (1023*255*63 < startingSector)
        {
            TEST(startingCHS == 0xFFC101);
        }
        if (1023*255*63 < startingSector + totalSector)
        {
            TEST(endingCHS == 0xFFFFFE);
        }

        u8 system = byte(entry + MBR_SystemIndicator);
        if (system == 0x05 || system == 0x0f)
        {
            extended = entry;
        }
    }

    if (!extended)
    {
        return;
    }

    u32 base = dword(extended + MBR_StartingSector);
    u32 epbr = base;
    while (extended)
    {
        disk->read(mbr, geometry.bytesPerSector, epbr * geometry.bytesPerSector);
        if (word(mbr + MBR_Signature) != 0xaa55)
        {
            break;
        }

        entry = &mbr[MBR_PartitionTable];

#ifdef VERBOSE
        esReport("%02x %06x %02x %06x %08x %08x %08x\n",
                 byte(entry + MBR_BootIndicator),
                 dword(entry + MBR_StartingCHS) & 0xffffff,
                 byte(entry + MBR_SystemIndicator),
                 dword(entry + MBR_EndingCHS) & 0xffffff,
                 dword(entry + MBR_StartingSector),
                 dword(entry + MBR_TotalSectors),
                 epbr + dword(entry + MBR_StartingSector));
#endif // VERBOSE

        startingCHS = dword(entry + MBR_StartingCHS) & 0xffffff;
        endingCHS = dword(entry + MBR_EndingCHS) & 0xffffff;
        startingSector = epbr + dword(entry + MBR_StartingSector);
        totalSector = dword(entry + MBR_TotalSectors);
        if (1023*255*63 < startingSector)
        {
            TEST(startingCHS == 0xFFC101);
        }
        if (1023*255*63 < startingSector + totalSector)
        {
            TEST(endingCHS == 0xFFFFFE);
        }

        u8 system = byte(entry + MBR_SystemIndicator);
        TEST(system != 0x05 && system != 0x0f);

        extended = &mbr[MBR_PartitionTable + 16];
        system = byte(entry + MBR_SystemIndicator);
        TEST(system == 0x05 || system == 0);

        if (byte(extended + MBR_SystemIndicator) != 0x05)
        {
            extended = 0;
        }
        else
        {
            epbr = base + dword(extended + MBR_StartingSector);
        }
    }
}

int main(int argc, char* argv[])
{
    Object* ns = 0;
    esInit(&ns);

    Handle<es::Stream> disk = new VDisk(static_cast<char*>("over8GB.img"));
    Handle<es::DiskManagement> dm = disk;
    dm->getGeometry(&VDiskGeometry);

    Handle<es::Partition> partition = new PartitionContext();
    TEST(partition);

    // mount
    TEST(partition->mount(disk) == 0);
    {
        Handle<es::Context> context = partition;
        TEST(context);

        Init(context);

        Test(context);
        CheckMbr(disk);
        TestExtended(context);

        // show results.
        esReport("\nPartition Entry Table\n");
        CheckMbr(disk);
        PrintPartitions(context);
    }

    // unmount
    TEST(partition->unmount() == 0);
    partition = 0;

    partition = new PartitionContext();
    TEST(partition);
    TEST(partition->mount(disk) == 0);
    {
        Handle<es::Context> context = partition;
        // check
        Handle<es::Stream> stream;
        TEST(stream = context->lookup("partition0"));
        TEST(stream = context->lookup("partition1"));
        TEST(!(stream = context->lookup("partition2")));
        TEST(stream = context->lookup("extended"));
        TEST(stream = context->lookup("logical0"));
        TEST(stream = context->lookup("logical1"));
        TEST(!(stream = context->lookup("logical2")));
    }
    // unmount
    TEST(partition->unmount() == 0);

    esReport("done.\n\n");
}
