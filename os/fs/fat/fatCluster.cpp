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
#include <es.h>
#include "fatStream.h"

u32 FatFileSystem::
calcSize(u32 clus)
{
    if (0 == clus)
    {
        if (!isFat32())
        {
            return 32 * word(bpb + BPB_RootEntCnt);
        }
        else
        {
            clus = dword(bpb + BPB_RootClus);
        }
    }

    u32 size = 0;
    while (!isEof(clus))
    {
        if (clus < 2 || isBadCluster(clus) || countOfClusters + 2 <= clus)
        {
            break;
        }
        size += bytsPerClus;
        clus = clusEntryVal(clus);
        if (DIR_LIMIT < size)       // XXX assume directory
        {
            break;
        }
    }
    return size;
}

int FatFileSystem::
readCluster(void* dst, int count, u32 clus, int offset)
{
    u32 secNum;

    ASSERT(offset % bytsPerSec == 0);

    if (count <= 0)
    {
        return 0;
    }
    count = (count + bytsPerSec - 1) & ~(bytsPerSec - 1);

    if (clus == 0)
    {
        // The root directory of FAT12/FAT16 file system.
        ASSERT(!isFat32());
        u32 size = calcSize(clus);
        if (size <= offset)
        {
            return 0;
        }
        if (size - offset < count)
        {
            count = size - offset;
        }
        secNum = firstRootDirSector + offset / bytsPerSec;
    }
    else
    {
        if (bytsPerClus <= offset)
        {
            return 0;
        }
        secNum = firstSectorOfCluster(clus) + (offset % bytsPerClus) / bytsPerSec;
        int len = bytsPerClus - offset;
        while (len < count)
        {
            u32 next = clusEntryVal(clus);
            if (++clus != next)
            {
                break;
            }
            len += bytsPerClus;
        }
        if (len < count)
        {
            count = len;
        }
    }

    return partition->read(dst, count, secNum * bytsPerSec);
}

int FatFileSystem::
writeCluster(const void* src, int count, u32 clus, int offset)
{
    u32 secNum;

    ASSERT(offset % bytsPerSec == 0);

    if (count <= 0)
    {
        return 0;
    }
    count = (count + bytsPerSec - 1) & ~(bytsPerSec - 1);

    if (clus == 0)
    {
        // The root directory of FAT12/FAT16 file system.
        ASSERT(!isFat32());
        u32 size = calcSize(clus);
        if (size <= offset)
        {
            return 0;
        }
        if (size - offset < count)
        {
            count = size - offset;
        }
        secNum = firstRootDirSector + offset / bytsPerSec;
    }
    else
    {
        if (bytsPerClus <= offset)
        {
            return 0;
        }
        secNum = firstSectorOfCluster(clus) + (offset % bytsPerClus) / bytsPerSec;
        int len = bytsPerClus - offset;
        while (len < count)
        {
            u32 next = clusEntryVal(clus);
            if (++clus != next)
            {
                break;
            }
            len += bytsPerClus;
        }
        if (len < count)
        {
            count = len;
        }
    }

    return partition->write(src, count, secNum * bytsPerSec);
}

int FatFileSystem::
zeroCluster(u32 clus)
{
    int len;
    int n;

    ASSERT(clus != 0);
    for (len = 0; len < bytsPerClus; len += n)
    {
        n = writeCluster(zero, bytsPerClus - len, clus, len);
        if (n <= 0)
        {
            break;
        }
    }
    return len;
}

u32 FatFileSystem::
allocCluster(u32 n, bool zero)
{
    Synchronized<es::Monitor*> method(fatMonitor);

    if (n == 0 || freeCount < n)
    {
        return 0xffffffff;
    }
    u32 clus = 0;
    u32 prev = 0;
    u32 count = 0;
    while (0 < n)
    {
        if (isEof(nxtFree))
        {
            nxtFree = 2;
        }
        if (clusEntryVal(nxtFree) == 0)
        {
            setClusEntryVal(nxtFree, 0xffffffff);
            --freeCount;
            if (zero)
            {
                zeroCluster(nxtFree);   // XXX
            }
            if (prev)
            {
                setClusEntryVal(prev, nxtFree);
            }
            else
            {
                clus = nxtFree;
            }
            prev = nxtFree;
            --n;
        }
        if (countOfClusters < ++count)
        {
            freeCluster(clus);
            return 0xffffffff;
        }
        ++nxtFree;
    }
    return clus;
}

void FatFileSystem::
freeCluster(u32 clus)
{
    if (2 <= clus)
    {
        while (!isEof(clus))
        {
            u32 next = clusEntryVal(clus);
            setClusEntryVal(clus, 0);
            ++freeCount;
            clus = next;
        }
    }
}

u32 FatFileSystem::
clusEntryVal(u32 n)
{
    Synchronized<es::Monitor*> method(fatMonitor);

    u32 fatOffset;
    u8 buf[4];

    if (isFat12())
    {
        fatOffset = n + (n / 2);
    }
    else if (isFat16())
    {
        fatOffset = n * 2;
    }
    else
    {
        fatOffset = n * 4;
    }

    u32 thisFatSecNum = word(bpb + BPB_RsvdSecCnt) + (fatOffset / bytsPerSec);
    u32 thisFatOffset = fatOffset % bytsPerSec;
    s64 offset = thisFatSecNum * bytsPerSec + thisFatOffset;

    if (isFat12())
    {
        diskStream->read(buf, 2, offset);
        u32 val = word(buf);
        if (n & 1)
        {
            return val >> 4;
        }
        else
        {
            return val & 0xfff;
        }
    }
    else if (isFat16())
    {
        diskStream->read(buf, 2, offset);
        return word(buf);
    }
    else
    {
        diskStream->read(buf, 4, offset);
        return dword(buf);
    }
}

void FatFileSystem::
setClusEntryVal(u32 n, u32 v)
{
    Synchronized<es::Monitor*> method(fatMonitor);

    // ASSERT(2 <= n);
    ASSERT(!isEof(n));

    u32 fatOffset;

    if (isFat12())
    {
        fatOffset = n + (n / 2);
    }
    else if (isFat16())
    {
        fatOffset = n * 2;
    }
    else
    {
        fatOffset = n * 4;
    }

    u32 thisFatSecNum = word(bpb + BPB_RsvdSecCnt) + (fatOffset / bytsPerSec);
    u32 thisFatOffset = fatOffset % bytsPerSec;
    s64 offset = thisFatSecNum * bytsPerSec + thisFatOffset;
    int len = bytsPerSec;
    u8 buf[4];

    if (isFat12())
    {
        diskStream->read(buf, 2, offset);
        if (n & 1)
        {
            v <<= 4;
            v |= word(buf) & 0x000f;
        }
        else
        {
            v &= 0xfff;
            v |= word(buf) & 0xf000;
        }
        xword(buf, v);
        for (n = bpb[BPB_NumFATs]; 0 < n; --n)
        {
            diskStream->write(buf, 2, offset);
            diskStream->flush();
            offset += fatSz * bytsPerSec;
        }
#if 0
        if (thisFatOffset == (bytsPerSec - 1))
        {
            len += bytsPerSec;
        }
#endif
    }
    else if (isFat16())
    {
        xword(buf, v);
        for (n = bpb[BPB_NumFATs]; 0 < n; --n)
        {
            diskStream->write(buf, 2, offset);
            diskStream->flush();
            offset += fatSz * bytsPerSec;
        }
    }
    else
    {
        diskStream->read(buf, 4, offset);
        v &= 0x0fffffff;
        v |= dword(buf) & 0xf0000000;
        xdword(buf, v);
        for (n = bpb[BPB_NumFATs]; 0 < n; --n)
        {
            diskStream->write(buf, 4, offset);
            diskStream->flush();
            offset += fatSz * bytsPerSec;
        }
    }
}
