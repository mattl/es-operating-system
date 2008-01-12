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
#include <es/endian.h>

using namespace LittleEndian;

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

IDiskManagement::Geometry VDiskGeometry;

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

void PrintBR(IStream* disk)
{
    IDiskManagement::Geometry geometry = VDiskGeometry;

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

    IInterface* ns = 0;
    esInit(&ns);

    Handle<IStream> disk = new VDisk(static_cast<char*>(argv[1]));
    Handle<IDiskManagement> dm = disk;
    dm->getGeometry(&VDiskGeometry);

    PrintBR(disk);

    Handle<IPartition> partition = new PartitionContext();
    TEST(partition);

    // mount
    TEST(partition->mount(disk) == 0);
    {
        esReport("mounted partitions.\n");
        Handle<IContext> context = partition;
        PrintPartitions(context);
    }
    // unmount
    TEST(partition->unmount() == 0);

    esReport("done.\n");
}
