/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

#include <errno.h>
#include <string.h>
#include <es.h>
#include "fatStream.h"

u8* FatFileSystem::zero;

bool FatFileSystem::
isFatPartition(u8 type)
{
    static const u8 fatTypes[] =
    {
        0x01, // FAT12
        0x04, // FAT16
        0x06, // FAT16
        0x0B, // FAT32
        0x0C, // FAT32
        0x0E, // FAT16
    };

    const u8* p;
    for (p = fatTypes; p < &fatTypes[sizeof fatTypes]; ++p)
    {
        if (*p == type)
        {
            return true;
        }
    }
    return false;
}

bool FatFileSystem::
isFat12()
{
    return countOfClusters < 4085;
}

bool FatFileSystem::
isFat16()
{
    return 4085 <= countOfClusters && countOfClusters < 65525;
}

bool FatFileSystem::
isFat32()
{
    return 65525 <= countOfClusters;
}

u8 FatFileSystem::
getChecksum(u8* fcb)
{
    int len;
    u8 sum = 0;
    for (len = 0; len < 11; ++len)
    {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *fcb++;
    }
    return sum;
}

bool FatFileSystem::
isFreeEntry(u8* fcb)
{
    return (fcb[0] == 0xe5 || fcb[0] == 0x00) ? true : false;
}

bool FatFileSystem::
canRead(u8* fcb)
{
    return (fcb[DIR_Attr] & ATTR_VOLUME_ID) ? false : true;
}

bool FatFileSystem::
canWrite(u8* fcb)
{
    return (fcb[DIR_Attr] & (ATTR_VOLUME_ID | ATTR_READ_ONLY)) ? false : true;
}

bool FatFileSystem::
isDirectory(u8* fcb)
{
    return (fcb[DIR_Attr] & ATTR_DIRECTORY) ? true : false;
}

bool FatFileSystem::
isFile(u8* fcb)
{
    return (fcb[DIR_Attr] & (ATTR_VOLUME_ID | ATTR_DIRECTORY)) ? false : true;
}

bool FatFileSystem::
isHidden(u8* fcb)
{
    return (fcb[DIR_Attr] & ATTR_HIDDEN) ? true : false;
}

bool FatFileSystem::
isLongNameComponent(u8* fcb)
{
    return ((fcb[DIR_Attr] & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) ? true : false;
}

bool FatFileSystem::
isVolumeID(u8* fcb)
{
    return (fcb[DIR_Attr] & ATTR_VOLUME_ID) ? true : false;
}

int FatFileSystem::
hashCode(u32 dirClus, u32 offset)
{
    return (int) (dirClus * (512 / 32) + offset / 32);
}

bool FatFileSystem::
isClean()
{
    if (isFat16())
    {
        u32 val = clusEntryVal(1);
        return (val & FAT16_ClnShutBitMask) ? true : false;
    }
    else if (isFat32())
    {
        u32 val = clusEntryVal(1);
        return (val & FAT32_ClnShutBitMask) ? true : false;
    }
    else
    {
        return true;
    }
}

void FatFileSystem::
setClean(bool clean)
{
    if (isFat16())
    {
        u32 val = clusEntryVal(1);
        if (clean)
        {
            val |= FAT16_ClnShutBitMask;
        }
        else
        {
            val &= ~FAT16_ClnShutBitMask;
        }
        setClusEntryVal(1, val);
    }
    else if (isFat32())
    {
        u32 val = clusEntryVal(1);
        if (clean)
        {
            val |= FAT32_ClnShutBitMask;
        }
        else
        {
            val &= ~FAT32_ClnShutBitMask;
        }
        setClusEntryVal(1, val);
    }
}

FatStream* FatFileSystem::
getRoot()
{
    root->addRef();
    return root;
}

// The reference count of the looked up stream shall be incremented by one.
FatStream* FatFileSystem::
lookup(u32 dirClus, u32 offset)
{
    Synchronized<es::Monitor*> method(hashMonitor);

    FatStream* stream;
    FatStreamChain::Iterator iter =
        hashTable[hashCode(dirClus, offset) % hashSize].begin();
    while ((stream = iter.next()))
    {
        if (stream->dirClus == dirClus && stream->offset == offset && !stream->isRemoved())
        {
            stream->addRef();
            return stream;
        }
    }
    return 0;
}

void FatFileSystem::
add(FatStream* stream)
{
    Synchronized<es::Monitor*> method(hashMonitor);

    if (!stream->isRoot())
    {
        hashTable[stream->hashCode() % hashSize].addFirst(stream);
    }
}

void FatFileSystem::
remove(FatStream* stream)
{
    Synchronized<es::Monitor*> method(hashMonitor);

    if (!stream->isRoot() && !stream->isRemoved())
    {
        hashTable[stream->hashCode() % hashSize].remove(stream);
    }
}

void FatFileSystem::
activate(FatStream* stream)
{
    Synchronized<es::Monitor*> method(hashMonitor);

    ASSERT(standbyList.contains(stream));
    standbyList.remove(stream);
}

void FatFileSystem::
standBy(FatStream* stream)
{
    Synchronized<es::Monitor*> method(hashMonitor);

    ASSERT(!standbyList.contains(stream));
    standbyList.addLast(stream);
    // XXX Maintain standbyList
}

// Returns the first sector number of the first sector of cluster 'n'.
// '0' represents the root directory.
u32 FatFileSystem::
firstSectorOfCluster(u32 n)
{
    if (0 < n)
    {
        return ((n - 2) * byte(bpb + BPB_SecPerClus)) + firstDataSector;
    }
    else
    {
        return firstRootDirSector;
    }
}

bool FatFileSystem::
isEof(u32 clus)
{
    return (countOfClusters + 1 < clus) ? true : false;
}

bool FatFileSystem::
isBadCluster(u32 clus)
{
    if (isFat12())
    {
        if (0x0FF7 == clus)
        {
            return true;
        }
    }
    else if (isFat16())
    {
        if (0xFFF7 == clus)
        {
            return true;
        }
    }
    else if (0x0FFFFFF7 == clus)
    {
        return true;
    }
    return false;
}

void FatFileSystem::
init()
{
    hashSize = 20;
    hashTable = new FatStreamChain[hashSize];

    hashMonitor = es::Monitor::createInstance();
    fatMonitor =es::Monitor::createInstance();

    // We must reserve a few pages for diskCache so that
    // we can access to FAT to write back file streams
    // under any low memory condition.
    pageSet = es::PageSet::createInstance();
    pageSet->reserve(1);
}

FatFileSystem::
FatFileSystem() :
    partition(0),
    pageSet(0),
    diskCache(0),
    diskStream(0),
    root(0),
    hashMonitor(0),
    fatMonitor(0),
    bytsPerSec(0),
    bytsPerClus(0),
    countOfClusters(0),
    freeCount(0)
{
    init();
}

FatFileSystem::
FatFileSystem(es::Stream* partition) :
    partition(0),
    pageSet(0),
    diskCache(0),
    diskStream(0),
    root(0),
    hashMonitor(0),
    fatMonitor(0),
    bytsPerSec(0),
    bytsPerClus(0),
    countOfClusters(0),
    freeCount(0)
{
    init();
    mount(partition);
}

FatFileSystem::
~FatFileSystem()
{
    dismount();

    pageSet->release();
    fatMonitor->release();
    hashMonitor->release();

    delete[] hashTable;
}

void FatFileSystem::
mount(es::Stream* disk)
{
    if (!disk)
    {
        return;
    }

    if (partition)
    {
        esThrow(EALREADY);
    }

    partition = disk;
    partition->addRef();

    int len = partition->read(bpb, 512, 0);
    // Signature 0xAA55
    esReport("Signature       0x%04x\n", word(bpb + 510));
    if (word(bpb + 510) != 0xaa55)
    {
        esThrow(EINVAL);
    }

    esReport("BS_jmpBoot      %02x %02x %02x\n", byte(bpb + BS_jmpBoot),
                                               byte(bpb + BS_jmpBoot + 1),
                                               byte(bpb + BS_jmpBoot + 2));
    esReport("BS_OEMName      %.8s\n", bpb + BS_OEMName);
    esReport("BPB_BytsPerSec  %u\n", word(bpb + BPB_BytsPerSec));
    esReport("BPB_SecPerClus  %u\n", byte(bpb + BPB_SecPerClus));
    esReport("BPB_RsvdSecCnt  %u\n", word(bpb + BPB_RsvdSecCnt));
    esReport("BPB_NumFATs     %u\n", byte(bpb + BPB_NumFATs));
    esReport("BPB_RootEntCnt  %u\n", word(bpb + BPB_RootEntCnt));
    esReport("BPB_TotSec16    %u\n", word(bpb + BPB_TotSec16));
    esReport("BPB_Media       %u\n", byte(bpb + BPB_Media));
    esReport("BPB_FATSz16     %u\n", word(bpb + BPB_FATSz16));
    esReport("BPB_SecPerTrk   %u\n", word(bpb + BPB_SecPerTrk));
    esReport("BPB_NumHeads    %u\n", word(bpb + BPB_NumHeads));
    esReport("BPB_HiddSec     %u\n", dword(bpb + BPB_HiddSec));
    esReport("BPB_TotSec32    %u\n", dword(bpb + BPB_TotSec32));

    bytsPerSec = word(bpb + BPB_BytsPerSec);
    bytsPerClus = bytsPerSec * byte(bpb + BPB_SecPerClus);
    hiddSec = dword(bpb + BPB_HiddSec);

    // FAT Type Determination
    rootDirSectors = ((word(bpb + BPB_RootEntCnt) * 32) +
                      (bytsPerSec - 1)) / bytsPerSec;
    fatSz = word(bpb + BPB_FATSz16);
    if (fatSz == 0)
    {
        fatSz = dword(bpb + BPB_FATSz32);
    }
    totSec = word(bpb + BPB_TotSec16);
    if (totSec == 0)
    {
        totSec = dword(bpb + BPB_TotSec32);
    }

    firstRootDirSector = word(bpb + BPB_RsvdSecCnt) +
                         byte(bpb + BPB_NumFATs) * fatSz;

    firstDataSector = firstRootDirSector + rootDirSectors;

    dataSec = totSec - firstDataSector;
    countOfClusters = dataSec / byte(bpb + BPB_SecPerClus);

    if (isFat12())
    {
        esReport("Volume is FAT12 (%d)\n", countOfClusters);
    }
    else if (isFat16())
    {
        esReport("Volume is FAT16 (%d)\n", countOfClusters);
    }
    else if (isFat32())
    {
        esReport("Volume is FAT32 (%d)\n", countOfClusters);
        firstRootDirSector = firstSectorOfCluster(dword(bpb + BPB_RootClus));
    }
    if (!isFat32())
    {
        esReport("BS_DrvNum       0x%x\n", byte(bpb + BS_DrvNum));
        esReport("BS_Reserved1    %u\n", byte(bpb + BS_Reserved1));
        esReport("BS_BootSig      %u\n", byte(bpb + BS_BootSig));
        esReport("BS_VolID        %u\n", dword(bpb + BS_VolID));
        esReport("BS_VolLab       %.11s\n", bpb + BS_VolLab);
        esReport("BS_FilSysType   %.8s\n", bpb + BS_FilSysType);

        nxtFree = 0xffffffff;
        freeCount = 0xffffffff;
    }
    else
    {
        esReport("BPB_FATSz32     %u\n", dword(bpb + BPB_FATSz32));
        esReport("BPB_ExtFlags    %u\n", word(bpb + BPB_ExtFlags));
        esReport("BPB_FSVer       %u\n", word(bpb + BPB_FSVer));
        esReport("BPB_RootClus    %u\n", dword(bpb + BPB_RootClus));
        esReport("BPB_FSInfo      %u\n", word(bpb + BPB_FSInfo));
        esReport("BPB_BkBootSec   %u\n", word(bpb + BPB_BkBootSec));
        esReport("BS_DrvNum       0x%x\n", byte(bpb + BS32_DrvNum));
        esReport("BS_Reserved1    %u\n", byte(bpb + BS32_Reserved1));
        esReport("BS_BootSig      %u\n", byte(bpb + BS32_BootSig));
        esReport("BS_VolID        %u\n", dword(bpb + BS32_VolID));
        esReport("BS_VolLab       %.11s\n", bpb + BS32_VolLab);
        esReport("BS_FilSysType   %.8s\n", bpb + BS32_FilSysType);

        u32 fsInfo = word(bpb + BPB_FSInfo);
        len = partition->read(fsi, 512, fsInfo * bytsPerSec);
        esReport("FSI_LeadSig     0x%08x\n", dword(fsi + FSI_LeadSig));
        esReport("FSI_StrucSig    0x%08x\n", dword(fsi + FSI_StrucSig));
        esReport("FSI_Free_Count  %u\n", dword(fsi + FSI_Free_Count));
        esReport("FSI_Nxt_Free    %u\n", dword(fsi + FSI_Nxt_Free));
        esReport("FSI_TrailSig    0x%08x\n", dword(fsi + FSI_TrailSig));

        if (dword(fsi + FSI_LeadSig) == 0x41615252 &&
            dword(fsi + FSI_StrucSig) == 0x61417272 &&
            dword(fsi + FSI_TrailSig) == 0xaa550000)
        {
            esReport("found FAT32 FSInfo.\n");
            nxtFree = dword(fsi + FSI_Nxt_Free);
            freeCount = dword(fsi + FSI_Free_Count);
        }
        else
        {
            esReport("not found FAT32 FSInfo.\n");
            nxtFree = 0xffffffff;
            freeCount = 0xffffffff;
        }
    }

    zero = new u8[bytsPerClus]; // XXX
    memset(zero, 0, bytsPerClus);

    diskCache = es::Cache::createInstance(partition, pageSet);
    diskCache->setSectorSize(bytsPerSec);
    diskStream = diskCache->getStream();

    // Check and clear ClnShutBitMask in FAT[1] for FAT16 and FAT32.
    if (!isClean())
    {
        esReport("This file system was not dismounted the last time it is mounted.\n");
        nxtFree = 0xffffffff;
        freeCount = 0xffffffff;
    }
    else
    {
        setClean(false);
    }

    // Calculate nxtFree and freeCount if necessary.
    if (isEof(nxtFree) || isEof(freeCount))
    {
        u32 n;

        freeCount = 0;
        for (n = 2; !isEof(n); ++n)
        {
            if (clusEntryVal(n) == 0)
            {
                ++freeCount;
                if (isEof(nxtFree))
                {
                    nxtFree = n;
                }
            }
        }
    }
    esReport("freeCount: %u (%lluKB)\n", freeCount, ((u64) freeCount * bytsPerClus) / 1024);
    esReport("nxtFree:   %u\n", nxtFree);

    // Create the root node
    u8 fcb[32];
    memset(fcb, ' ', 11);
    memset(fcb + 11, 0, 32 - 11);
    xbyte(fcb + DIR_Attr, ATTR_DIRECTORY);
    if (isFat32())
    {
        xword(fcb + DIR_FstClusLO, dword(bpb + BPB_RootClus));
        xword(fcb + DIR_FstClusHI, dword(bpb + BPB_RootClus) >> 16);
    }
    root = new FatStream(this, 0, 0, fcb);
}

void FatFileSystem::
dismount(void)
{
    if (!partition)
    {
        return;
    }

    if (root)
    {
        root->flush();
        root->release();
        root = 0;

        // XXX Must check every stream has been closed.

        while (!standbyList.isEmpty())
        {
            FatStream* stream = standbyList.removeFirst();

            remove(stream);
        }

        if (isFat32())
        {
            // Update FSInfo sector.
            if (dword(fsi + FSI_LeadSig) == 0x41615252 &&
                dword(fsi + FSI_StrucSig) == 0x61417272 &&
                dword(fsi + FSI_TrailSig) == 0xaa550000)
            {
                xdword(fsi + FSI_Nxt_Free, nxtFree);
                xdword(fsi + FSI_Free_Count, freeCount);
                u32 fsInfo = word(bpb + BPB_FSInfo);
                partition->write(fsi, 512, fsInfo * bytsPerSec);
            }
        }

        // Set ClnShutBitMask in FAT[1] for FAT16 and FAT32.
        setClean(true);

        diskStream->flush();
        diskStream->release();
        diskCache->release();

        delete[] zero;
    }

    partition->release();
    partition = 0;

    bytsPerClus = 0;
    countOfClusters = freeCount = 0;
}

void FatFileSystem::
getRoot(es::Context** root)
{
    if (root)
    {
        *root = getRoot();
    }
}

long long FatFileSystem::
getFreeSpace()
{
    return (long long) freeCount * bytsPerClus;
}

long long FatFileSystem::
getTotalSpace()
{
    return (long long) countOfClusters * bytsPerClus;
}

int FatFileSystem::
checkDisk(bool fixError)
{
    return check();
}

void FatFileSystem::
format()
{
    if (!partition)
    {
        esThrow(EALREADY);
    }

    es::Stream* p = partition;
    p->addRef();

    dismount();
    format(p);
    mount(p);

    p->release();
}

int FatFileSystem::
defrag()
{
    return 0;
}

Object* FatFileSystem::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::FatFileSystem::iid()) == 0)
    {
        objectPtr = static_cast<es::FatFileSystem*>(this);
    }
    if (strcmp(riid, es::FileSystem::iid()) == 0)
    {
        objectPtr = static_cast<es::FatFileSystem*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::FatFileSystem*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int FatFileSystem::
addRef()
{
    return ref.addRef();
}

unsigned int FatFileSystem::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}

es::FatFileSystem* FatFileSystem::Constructor::
createInstance()
{
    return new FatFileSystem;
}

Object* FatFileSystem::Constructor::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::FatFileSystem::Constructor::iid()) == 0)
    {
        objectPtr = static_cast<es::FatFileSystem::Constructor*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::FatFileSystem::Constructor*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int FatFileSystem::Constructor::
addRef()
{
    return 1;
}

unsigned int FatFileSystem::Constructor::
release()
{
    return 1;
}

void FatFileSystem::
initializeConstructor()
{
    static Constructor constructor;
    es::FatFileSystem::setConstructor(&constructor);
}