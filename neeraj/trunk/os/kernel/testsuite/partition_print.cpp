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

using namespace LittleEndian;

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

es::DiskManagement::Geometry VDiskGeometry;

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

void PrintBR(es::Stream* disk)
{
    es::DiskManagement::Geometry geometry = VDiskGeometry;

    // Read MBR
    u8 mbr[geometry.bytesPerSector];
    disk->read(mbr, geometry.bytesPerSector, 0);
    TEST(word(mbr + MBR_Signature) == 0xaa55);
    esReport("MBR\n");
    esReport("Boot StartCHS Type EndCHS StartSec TotalSec\n");
    u8* entry = &mbr[MBR_PartitionTable];
    u8* extended = 0;
    int i;
    for (i = 0; i < 4; ++i, entry += MBR_EntrySize)
    {
        esReport("%02x   %06x   %02x   %06x %08x %08x\n",
                 byte(entry + MBR_BootIndicator),
                 dword(entry + MBR_StartingCHS) & 0xffffff,
                 byte(entry + MBR_SystemIndicator),
                 dword(entry + MBR_EndingCHS) & 0xffffff,
                 dword(entry + MBR_StartingSector),
                 dword(entry + MBR_TotalSectors));

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

    esReport("\nEPBR\n");
    esReport("Boot StartCHS Type EndCHS StartSec TotalSec StartSec(abs)\n");
    u32 base = dword(extended + MBR_StartingSector);
    u32 epbr = base;
    while (extended)
    {
        disk->read(mbr, geometry.bytesPerSector, (long long) epbr * geometry.bytesPerSector);
        if (word(mbr + MBR_Signature) != 0xaa55)
        {
            break;
        }

        entry = &mbr[MBR_PartitionTable];

        // check entry
        esReport("%02x   %06x   %02x   %06x %08x %08x %08x\n",
                 byte(entry + MBR_BootIndicator),
                 dword(entry + MBR_StartingCHS) & 0xffffff,
                 byte(entry + MBR_SystemIndicator),
                 dword(entry + MBR_EndingCHS) & 0xffffff,
                 dword(entry + MBR_StartingSector),
                 dword(entry + MBR_TotalSectors),
                 epbr +  dword(entry + MBR_StartingSector));
        extended = &mbr[MBR_PartitionTable + 16];

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

// This is a tool to print entries in MBR and EPBR.
// Specify an image file as an argument.
int main(int argc, char* argv[])
{
    if (argc < 2 || !argv[1])
    {
        return 0;
    }

    Object* ns = 0;
    esInit(&ns);

    Handle<es::Stream> disk = new VDisk(static_cast<char*>(argv[1]));
    Handle<es::DiskManagement> dm = disk;
    dm->getGeometry(&VDiskGeometry);

    PrintBR(disk);

    Handle<es::Partition> partition = new PartitionContext();
    TEST(partition);

    // mount
    TEST(partition->mount(disk) == 0);
    {
        esReport("mounted partitions.\n");
        Handle<es::Context> context = partition;
        PrintPartitions(context);
    }
    // unmount
    TEST(partition->unmount() == 0);

    esReport("done.\n");
}
