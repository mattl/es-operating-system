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

/*
 * These coded instructions, statements, and computer programs contain
 * software derived from the following specification:
 *
 * Microsoft, "Microsoft Extensible Firmware Initiative FAT32 File System
 * Specification," 6 Dec. 2000.
 * http://www.microsoft.com/whdc/system/platform/firmware/fatgen.mspx
 */

#include <string.h>
#include <stdio.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include "fatStream.h"

extern "C"
{
    extern unsigned char bootfat12[];
    extern unsigned char bootfat16[];
    extern unsigned char bootfat32[];
}

namespace
{
    struct DSKSZTOSECPERCLUS
    {
        u32 diskSize;
        u8  secPerClus;
    };

    /*
     * This is the table for FAT16 drives. NOTE that this table includes
     * entries for disk sizes larger than 512 MB even though typically
     * only the entries for disks < 512 MB in size are used.
     * The way this table is accessed is to look for the first entry
     * in the table for which the disk size is less than or equal
     * to the DiskSize field in that table entry.  For this table to
     * work properly BPB_RsvdSecCnt must be 1, BPB_NumFATs
     * must be 2, and BPB_RootEntCnt must be 512. Any of these values
     * being different may require the first table entries DiskSize value
     * to be changed otherwise the cluster count may be to low for FAT16.
     */
    DSKSZTOSECPERCLUS DskTableFAT16[] =
    {
           8400,    0,      // disks up to 4.1 MB, the 0 value for SecPerClusVal trips an error
          32680,    2,      // disks up to  16 MB,  1k cluster
         262144,    4,      // disks up to 128 MB,  2k cluster
         524288,    8,      // disks up to 256 MB,  4k cluster
        1048576,    16,     // disks up to 512 MB,  8k cluster
        // The entries after this point are not used unless FAT16 is forced
        2097152,    32,     // disks up to   1 GB, 16k cluster
        4194304,    64,     // disks up to   2 GB, 32k cluster
        0xFFFFFFFF, 0       // any disk greater than 2GB, 0 value for SecPerClusVal trips an error
    };

    /*
     * This is the table for FAT32 drives. NOTE that this table includes
     * entries for disk sizes smaller than 512 MB even though typically
     * only the entries for disks >= 512 MB in size are used.
     * The way this table is accessed is to look for the first entry
     * in the table for which the disk size is less than or equal
     * to the DiskSize field in that table entry. For this table to
     * work properly BPB_RsvdSecCnt must be 32, and BPB_NumFATs
     * must be 2. Any of these values being different may require the first
     * table entries DiskSize value to be changed otherwise the cluster count
     * may be to low for FAT32.
     */
    DSKSZTOSECPERCLUS DskTableFAT32[] =
    {
           66600,    0,     // disks up to 32.5 MB, the 0 value for SecPerClusVal trips an error
          532480,    1,     // disks up to  260 MB, .5k cluster
        16777216,    8,     // disks up to    8 GB,  4k cluster
        33554432,   16,     // disks up to   16 GB,  8k cluster
        67108864,   32,     // disks up to   32 GB,  16k cluster
        0xFFFFFFFF, 64      // disks greater than 32GB, 32k cluster
    };

}

int FatFileSystem::
updateBootCode()
{
    Geometry geometry;

    getGeometry(partition, &geometry);

    u8* sector = new u8[512];

    partition->read(sector, 512, 0);

    // update boot code
    if (isFat12())
    {
        memmove(sector, bootfat12, 3);
        memmove(sector + BS_FilSysType + 8,
                bootfat12 + BS_FilSysType + 8,
                512 - (BS_FilSysType + 8));
    }
    else if (isFat16())
    {
        memmove(sector, bootfat16, 3);
        memmove(sector + BS_FilSysType + 8,
                bootfat16 + BS_FilSysType + 8,
                512 - (BS_FilSysType + 8));
    }
    else if (isFat32())
    {
        memmove(sector, bootfat32, 3);
        memmove(sector + BS32_FilSysType + 8,
                bootfat32 + BS32_FilSysType + 8,
                512 - (BS32_FilSysType + 8));
    }
    else
    {
        return -1;
    }

    partition->write(sector, 512, 0);
    partition->flush();

    delete[] sector;
    return 0;
}

int FatFileSystem::
format(es::Stream* partition)
{
    long long size;
    size = partition->getSize();
    esReport("partition size: %lld bytes\n", size);

    if ((1LL << 28) * 32 * 1024 < size)
    {
        // diskSize is too large.
        return -1;
    }

    if (size <= 512 * 8400)
    {
        // FAT12
        return formatFat12(partition);
    }
    else if (size < 512 * 1024 * 1024)
    {
        // FAT16
        return formatFat16(partition);
    }
    else
    {
        // FAT32
        return formatFat32(partition);
    }
}

void FatFileSystem::
getGeometry(es::Stream* partition, Geometry* geometry)
{
    Handle<es::Disk> disk(partition, true);
    if (disk)
    {
        try
        {
            geometry->heads = disk->getHeads();
            geometry->cylinders = disk->getCylinders();
            geometry->sectorsPerTrack = disk->getSectorsPerTrack();
            geometry->bytesPerSector = disk->getBytesPerSector();
            geometry->diskSize = disk->getDiskSize();
            if (geometry->diskSize < (512LL << 24) &&   // less than 8GB?
                geometry->sectorsPerTrack < (1 << 6))   // less than 64?
            {
                // Apply LARGE
                while (1023 < geometry->cylinders && geometry->heads < 128)
                {
                    geometry->cylinders /= 2;
                    geometry->heads *= 2;
                }
                if (1023 < geometry->cylinders || 127 < geometry->heads)
                {
                    geometry->cylinders = (geometry->cylinders * geometry->heads) / 255;
                    geometry->heads = 255;
                }
            }
        }
        catch (Exception& error)
        {
            // [check]
        }
    }
}

//   http://alumnus.caltech.edu/~pje/dosfiles.html
//
//   size                              5-1/4 5-1/4 5-1/4 5-1/4 5-1/4 3-1/2 3-1/2
//   density (D = double, H = high)      D     D     D     D     H     D     H
//   sides                               1     1     2     2     2     2     2
//   media descriptor byte              FE    FC    FF    FD    F9    F9    F0
//   bytes per sector                  512   512   512   512   512   512   512
//   sectors per cluster                 1     1     2     2     1     2     1
//   reserved sectors                    1     1     1     1     1     1     1
//   copies of file allocation table     2     2     2     2     2     2     2
//   entries in root directory          64    64   112   112   224   112   224
//   total sectors                     320   360   640   720  2400  1440  2880
//   sectors per file allocation table   1     2     1     2     7     3     9
//   sectors per track                   8     9     8     9    15     9    18
//   hidden sectors                      0     0     0     0     0     0     0

int FatFileSystem::
formatFat12(es::Stream* partition)
{
    Geometry geometry;
    u32 diskSize;   // total sectors
    u8  secPerClus; // sector per cluster
    u32 fatSz;
    u32 hiddSec = 0;

    getGeometry(partition, &geometry);
    diskSize = (u32) (geometry.diskSize / geometry.bytesPerSector);

    // XXX so far 3-1/2 2HD only
    if (diskSize != 2880 || geometry.bytesPerSector != 512)
    {
        return -1;
    }

    secPerClus = 1;
    fatSz = 9;

    u32 rootEntCnt = 224;
    u32 resvdSecCnt = 1;
    u32 numFATs = 2;
    u32 rootDirSectors = ((rootEntCnt * 32) + (geometry.bytesPerSector - 1)) / geometry.bytesPerSector;

    u8* sector = new u8[geometry.bytesPerSector];
    memset(sector, 0, geometry.bytesPerSector);

    // BPB
    memmove(sector, bootfat12, 512);
    xword(sector + BPB_BytsPerSec, geometry.bytesPerSector);
    xbyte(sector + BPB_SecPerClus, secPerClus);
    xword(sector + BPB_RsvdSecCnt, resvdSecCnt);
    xbyte(sector + BPB_NumFATs, numFATs);
    xword(sector + BPB_RootEntCnt, rootEntCnt);
    xword(sector + BPB_TotSec16, (diskSize >> 16) ? 0 : diskSize);
    xword(sector + BPB_FATSz16, fatSz);
    xword(sector + BPB_SecPerTrk, geometry.sectorsPerTrack);
    xword(sector + BPB_NumHeads, geometry.heads);
    xdword(sector + BPB_HiddSec, hiddSec);
    xdword(sector + BPB_TotSec32, (diskSize >> 16) ? diskSize : 0);
    xdword(sector + BS_VolID, 0);   // XXX
    partition->write(sector, geometry.bytesPerSector, 0);

    // FAT
    memset(sector, 0, geometry.bytesPerSector);
    sector[0] = byte(bootfat12 + BPB_Media);
    sector[1] = 0x8f;
    sector[2] = 0xff;
    for (int j = 0; j < numFATs; ++j)
    {
        partition->write(sector, geometry.bytesPerSector, (resvdSecCnt + j * fatSz) * geometry.bytesPerSector);
    }
    memset(sector, 0, 3);
    for (int i = 1; i < fatSz; ++i)
    {
        for (int j = 0; j < numFATs; ++j)
        {
            partition->write(sector, geometry.bytesPerSector, (resvdSecCnt + j * fatSz + i) * geometry.bytesPerSector);
        }
    }

    // Root Directory
    memset(sector, 0, geometry.bytesPerSector);
    for (int i = 0; i < rootDirSectors; ++i)
    {
        partition->write(sector, geometry.bytesPerSector, (resvdSecCnt + numFATs * fatSz + i) * geometry.bytesPerSector);
    }

    delete[] sector;

    return 0;
}

int FatFileSystem::
formatFat16(es::Stream* partition)
{
    Geometry geometry;
    u32 diskSize;   // total sectors
    u8  secPerClus; // sector per cluster
    u32 fatSz;
    u32 hiddSec = 0;

    getGeometry(partition, &geometry);
    diskSize = (u32) (geometry.diskSize / geometry.bytesPerSector);
    DSKSZTOSECPERCLUS* entry;
    for (entry = DskTableFAT16;
         entry->diskSize < diskSize;
         ++entry)
    {
    }
    secPerClus = entry->secPerClus;

    u32 rootEntCnt = 512;
    u32 resvdSecCnt = 1;
    u32 numFATs = 2;
    int diff;
    u32 rootDirSectors = ((rootEntCnt * 32) + (geometry.bytesPerSector - 1)) / geometry.bytesPerSector;
    u32 tmpVal1 = diskSize - (resvdSecCnt + rootDirSectors);
    u32 tmpVal2 = (256 * secPerClus) + numFATs;
    fatSz = (tmpVal1 + 2 * secPerClus + (tmpVal2 - 1)) / tmpVal2;
    diff = fatSz - ((tmpVal1 - numFATs * fatSz) / secPerClus + 2 + geometry.bytesPerSector / 2 - 1) /
                   (geometry.bytesPerSector / 2);
    ASSERT(0 <= diff);

    Handle<es::Disk> disk(partition, true);
    // TODO: Adjust hiddSec if a partition is used
    // hiddSec = layout.startingOffset / geometry.bytesPerSector;

    u8* sector = new u8[geometry.bytesPerSector];
    memset(sector, 0, geometry.bytesPerSector);

    // BPB
    memmove(sector, bootfat16, 512);
    xword(sector + BPB_BytsPerSec, geometry.bytesPerSector);
    xbyte(sector + BPB_SecPerClus, secPerClus);
    xword(sector + BPB_RsvdSecCnt, resvdSecCnt);
    xbyte(sector + BPB_NumFATs, numFATs);
    xword(sector + BPB_RootEntCnt, rootEntCnt);
    xword(sector + BPB_TotSec16, (diskSize >> 16) ? 0 : diskSize);
    xword(sector + BPB_FATSz16, fatSz);
    xword(sector + BPB_SecPerTrk, geometry.sectorsPerTrack);
    xword(sector + BPB_NumHeads, geometry.heads);
    xdword(sector + BPB_HiddSec, hiddSec);
    xdword(sector + BPB_TotSec32, (diskSize >> 16) ? diskSize : 0);
    xdword(sector + BS_VolID, 0);   // XXX
    partition->write(sector, geometry.bytesPerSector, 0);

    // FAT
    memset(sector, 0, geometry.bytesPerSector);
    xword(sector + 0, 0xff00 | byte(bootfat16 + BPB_Media));
    xword(sector + 2, 0xfff8 | FAT16_ClnShutBitMask | FAT16_HrdErrBitMask);
    for (int j = 0; j < numFATs; ++j)
    {
        partition->write(sector, geometry.bytesPerSector, (resvdSecCnt + j * fatSz) * geometry.bytesPerSector);
    }
    xword(sector + 0, 0);
    xword(sector + 2, 0);
    for (int i = 1; i < fatSz; ++i)
    {
        for (int j = 0; j < numFATs; ++j)
        {
            partition->write(sector, geometry.bytesPerSector, (resvdSecCnt + j * fatSz + i) * geometry.bytesPerSector);
        }
    }

    // Root Directory
    memset(sector, 0, geometry.bytesPerSector);
    for (int i = 0; i < rootDirSectors; ++i)
    {
        partition->write(sector, geometry.bytesPerSector, (resvdSecCnt + numFATs * fatSz + i) * geometry.bytesPerSector);
    }

    delete[] sector;

    return 0;
}

int FatFileSystem::
formatFat32(es::Stream* partition)
{
    Geometry geometry;
    u32 diskSize;   // total sectors
    u8  secPerClus; // sector per cluster
    u32 fatSz;
    u32 hiddSec = 0;

    getGeometry(partition, &geometry);
    diskSize = (u32) (geometry.diskSize / geometry.bytesPerSector);
    DSKSZTOSECPERCLUS* entry;
    for (entry = DskTableFAT32;
         entry->diskSize < diskSize;
         ++entry)
    {
    }
    secPerClus = entry->secPerClus;

    u32 rootEntCnt = 0;
    u32 resvdSecCnt = 36;
    u32 numFATs = 2;
    int diff;
    u32 rootDirSectors = ((rootEntCnt * 32) + (geometry.bytesPerSector - 1)) / geometry.bytesPerSector;
    u32 tmpVal1 = diskSize - (resvdSecCnt + rootDirSectors);
    u32 tmpVal2 = (128 * secPerClus) + numFATs;
    fatSz = (tmpVal1 + 2 * secPerClus + (tmpVal2 - 1)) / tmpVal2;
    diff = fatSz - ((tmpVal1 - numFATs * fatSz) / secPerClus + 2 + geometry.bytesPerSector / 4 - 1) /
                   (geometry.bytesPerSector / 4);
    ASSERT(0 <= diff);

    Handle<es::Disk> disk(partition, true);
    // TODO: Adjust hiddSec if a partition is used
    // hiddSec = layout.startingOffset / geometry.bytesPerSector;

    u8* sector = new u8[geometry.bytesPerSector];
    memset(sector, 0, geometry.bytesPerSector);

    // BPB and BkBootSec
    memmove(sector, bootfat32, 512);
    xword(sector + BPB_BytsPerSec, geometry.bytesPerSector);
    xbyte(sector + BPB_SecPerClus, secPerClus);
    xword(sector + BPB_RsvdSecCnt, resvdSecCnt);
    xbyte(sector + BPB_NumFATs, numFATs);
    xword(sector + BPB_RootEntCnt, rootEntCnt);
    xword(sector + BPB_TotSec16, (diskSize >> 16) ? 0 : diskSize);
    xword(sector + BPB_FATSz16, 0);
    xword(sector + BPB_SecPerTrk, geometry.sectorsPerTrack);
    xword(sector + BPB_NumHeads, geometry.heads);
    xdword(sector + BPB_HiddSec, hiddSec);
    xdword(sector + BPB_TotSec32, (diskSize >> 16) ? diskSize : 0);
    xdword(sector + BPB_FATSz32, fatSz);
    xdword(sector + BS32_VolID, 0); // XXX
    partition->write(sector, geometry.bytesPerSector, 0);
    partition->write(sector, geometry.bytesPerSector, word(sector + BPB_BkBootSec) * geometry.bytesPerSector);

    // FSInfo
    memmove(sector, bootfat32, 512);
    xdword(sector + FSI_LeadSig, 0x41615252);
    xdword(sector + FSI_StrucSig, 0x61417272);
    xdword(sector + FSI_Free_Count, (tmpVal1 - numFATs * fatSz) / secPerClus - 1);
    xdword(sector + FSI_Nxt_Free, 3);
    xdword(sector + FSI_TrailSig, 0xaa550000);
    partition->write(sector, geometry.bytesPerSector, 1 * geometry.bytesPerSector); // XXX 1

    // FAT
    memset(sector, 0, geometry.bytesPerSector);
    xdword(sector + 0, 0x0fffff00 | byte(bootfat32 + BPB_Media));
    xdword(sector + 4, 0x0ffffff8 | FAT32_ClnShutBitMask | FAT32_HrdErrBitMask);
    xdword(sector + 8, 0x0ffffff8); // root
    for (int j = 0; j < numFATs; ++j)
    {
        partition->write(sector, geometry.bytesPerSector, (resvdSecCnt + j * fatSz) * geometry.bytesPerSector);
    }
    xdword(sector + 0, 0);
    xdword(sector + 4, 0);
    xdword(sector + 8, 0);
    for (int i = 1; i < fatSz; ++i)
    {
        for (int j = 0; j < numFATs; ++j)
        {
            partition->write(sector, geometry.bytesPerSector, (resvdSecCnt + j * fatSz + i) * geometry.bytesPerSector);
        }
    }

    // Root Directory
    memset(sector, 0, geometry.bytesPerSector);
    for (int i = 0; i < secPerClus; ++i)
    {
        partition->write(sector, geometry.bytesPerSector, (resvdSecCnt + numFATs * fatSz + i) * geometry.bytesPerSector);
    }

    delete[] sector;

    return 0;
}
