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

// Disk partition manager. Since this manager can be applied to both
// ATA hard disks and SCSI hard disks, this manager is implemented
// separately from the hard disk device drivers.

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include "partition.h"

using namespace LittleEndian;

#define VERBOSE

const char* PartitionContext::PREFIX_PRIMARY = "partition";
const char* PartitionContext::PREFIX_EXTENDED = "extended";
const char* PartitionContext::PREFIX_LOGICAL = "logical";
const int PartitionContext::MAX_PREFIX_LEN = 9;

extern "C"
{
    extern unsigned char mbr[];
}

PartitionContext::
PartitionContext() : disk(0)
{

}

PartitionContext::
~PartitionContext()
{
    if (disk)
    {
        unmount();
    }
}

int PartitionContext::
convertLBAtoCHS(unsigned startingSector, unsigned totalSectors,
        unsigned& startingCHS, unsigned& endingCHS)
{
    PartitionStream::Geometry geometry;
    getGeometry(&geometry);

    unsigned cylinder;
    u8 head;
    u8 sector;
    if (startingSector <= MAX_CHS)
    {
        u32 heads;
        u32 cylinders;
        u32 sectorsPerTrack;
        u32 bytesPerSector;

        cylinder = startingSector / geometry.sectorsPerTrack / geometry.heads;
        head = (startingSector / geometry.sectorsPerTrack) % geometry.heads;
        sector = startingSector % geometry.sectorsPerTrack + 1;

#ifdef VERBOSE
        esReport("startingSector %x <---> c %x, h %x, s %x\n",
            startingSector, cylinder, head, sector);
#endif // VERBOSE
        // Partitions start at the second track of the first cylinder or
        // at cylinder boundaries.
        if (!(head == 1 && sector == 1) && !(head == 0 && sector == 1))
        {
            return -1;
        }

        startingCHS = (0xff & cylinder) << 16 | (0x0300 & cylinder) << 6 |
            sector << 8 | head;

        unsigned endingSector = startingSector + totalSectors - 1;
        if (endingSector <= MAX_CHS)
        {
            cylinder = endingSector / geometry.sectorsPerTrack / geometry.heads;
            head = (endingSector / geometry.sectorsPerTrack) % geometry.heads;
            sector = endingSector % geometry.sectorsPerTrack + 1;

#ifdef VERBOSE
        esReport("endingSector %x <---> c %x, h %x (%x), s %x (%x)\n",
            endingSector, cylinder, head, geometry.heads, sector, geometry.sectorsPerTrack);
#endif // VERBOSE
            if (!(head == geometry.heads - 1 && sector == geometry.sectorsPerTrack))
            {
                return -1;
            }
            endingCHS = (0xff & cylinder) << 16 | (0x0300 & cylinder) << 6 |
                sector << 8 | head;
        }
        else
        {
            endingCHS = MAX_ENDING_CHS;
        }
    }
    else
    {
        startingCHS = MAX_STARTING_CHS;
        endingCHS = MAX_ENDING_CHS;
    }

    return 0;
}

PartitionStream* PartitionContext::
createPartition(const char* name, u8 type)
{
    int id = getId(name, getPrefix(name));
    if (id < 0  || 3 < id || (type == TYPE_EXTENDED && id != 0))
    {
        return 0;
    }

    if (type == TYPE_PRIMARY && 0 < id &&
        lookupPartitionStream(type, id - 1) == 0)
    {
        // A primary partition must be created in numerical order.
        return 0;
    }

    es::DiskManagement::Geometry geometry;
    getGeometry(&geometry);

    // Read MBR
    u8 mbr[geometry.bytesPerSector];
    disk->read(mbr, geometry.bytesPerSector, 0);
    if (word(mbr + MBR_Signature) != MBR_SIGNATURE)
    {
        esReport("Not found MBR.\n");
        return 0;
    }

    u8* entry = getEntry(mbr, TYPE_EMPTY, 0);
    if (!entry)
    {
        // no empty partition entry.
        return 0;
    }

    u8 entryNo = (entry - &mbr[MBR_PartitionTable]) / MBR_EntrySize;

    unsigned prevOffset;
    unsigned prevSize;
    if (entryNo == 0)
    {
        prevOffset = geometry.sectorsPerTrack;
        prevSize = 0;
    }
    else
    {
        u8* prev = entry - MBR_EntrySize;
        prevOffset = dword(prev + MBR_StartingSector);
        prevSize = dword(prev + MBR_TotalSectors);
    }

    // Create a stream for the partition.
    PartitionStream* stream = new PartitionStream(this, id, entry, entryNo);
    if (!stream)
    {
        return 0;
    }
    switch (type)
    {
      case TYPE_PRIMARY:
        partitionList.addFirst(stream);
        break;

      case TYPE_EXTENDED:
        partitionList.addLast(stream);
        break;
    }

    PartitionStream::Partition partition;
    partition.startingOffset = (long long) (prevOffset + prevSize) * geometry.bytesPerSector;
    partition.hiddenSectors = 0;
    partition.bootIndicator = 0;

    partition.partitionLength = (long long) geometry.heads *
        geometry.sectorsPerTrack * geometry.bytesPerSector;
    stream->adjustSize(&geometry, &partition, partition.partitionLength);

    switch (type)
    {
      case TYPE_EXTENDED:
        if (MAX_CHS <= (long long) prevOffset + (long long) prevSize +
            partition.partitionLength / geometry.bytesPerSector)
        {
            partition.partitionType = PT_EXTENDED_LBA;
        }
        else
        {
            partition.partitionType = PT_EXTENDED;
        }
        stream->setType(TYPE_EXTENDED);
        break;
      case TYPE_PRIMARY:
        partition.partitionType = getDefaultPartitionType(partition.partitionLength);
        stream->setType(TYPE_PRIMARY);
        break;
    }

    stream->setLayout(&partition);
    return stream;
}

int PartitionContext::
initEPBR(u8* buf, u32 len, long long offset)
{
    memset(buf, 0, len);
    xword(buf + MBR_Signature, MBR_SIGNATURE);

    disk->write(buf, len, offset);
    disk->flush();

    return 0;
}

int PartitionContext::
getEPBROffset(PartitionStream* last, long long& offset)
{
    if (last->isExtendedPartition())
    {
        last->getOffset(offset);
    }
    else if (last->isLogicalPartition())
    {
        long long size;
        last->getOffset(offset);
        size = last->getSize();
        offset += size;
    }
    else
    {
        return -1;
    }

    return 0;
}

PartitionStream* PartitionContext::
createLogicalPartition(const char* name)
{
    int id = getId(name, getPrefix(name));
    if (id < 0 || 255 < id)
    {
        return 0;
    }

    // check the last partition.
    PartitionStream* last = partitionList.getLast();
    if (!last || !(last->isExtendedPartition() || last->isLogicalPartition()))
    {
        return 0;
    }

    // check id.
    if (!((last->isExtendedPartition() && id == 0) ||
          (last->isLogicalPartition() && id == last->getId() + 1)))
    {
        return 0;
    }

    long long offset;
    if (getEPBROffset(last, offset) < 0)
    {
        return 0;
    }

    es::DiskManagement::Geometry geometry;
    getGeometry(&geometry);
    u32 secSize = geometry.bytesPerSector;
    unsigned secPerCylinder = geometry.heads * geometry.sectorsPerTrack;

    // create EPBR
    u8 epbr[secSize];
    initEPBR(epbr, sizeof(epbr), offset);

    // Create a stream for the new logical partition.
    PartitionStream* stream;
    stream = new PartitionStream(this, id, &epbr[MBR_PartitionTable], 0, offset / secSize, last);
    if (!stream)
    {
        return 0;
    }
    partitionList.addLast(stream);

    PartitionStream::Partition partition;
    partition.startingOffset = offset + (long long) geometry.sectorsPerTrack * secSize;
    partition.partitionLength = (long long) geometry.heads *
        geometry.sectorsPerTrack * geometry.bytesPerSector;
    partition.partitionType = getDefaultPartitionType(partition.partitionLength);
    partition.hiddenSectors = 0;
    partition.bootIndicator = 0;

    try
    {
        stream->setLayout(&partition);
    }
    catch (Exception& error)
    {
        partitionList.remove(stream);
        stream->release();
        delete stream;
        return 0;
    }

    return stream;
}

int PartitionContext::
getGeometry(es::DiskManagement::Geometry* geometry)
{
    if (!disk || !geometry)
    {
        return -1;
    }
    Handle<es::DiskManagement> dm(disk, true);
    if (dm)
    {
        try
        {
            dm->getGeometry(geometry);
            return 0;
        }
        catch (Exception& error)
        {
        }
    }
    return -1;
}

const char* PartitionContext::
getPrefix(const char* name)
{
    const char* prefix = 0;
    if (memcmp(name, PREFIX_PRIMARY, strlen(PREFIX_PRIMARY)) == 0)
    {
        prefix = PREFIX_PRIMARY;
    }
    else if (memcmp(name, PREFIX_EXTENDED, strlen(PREFIX_EXTENDED)) == 0)
    {
        prefix = PREFIX_EXTENDED;
    }
    else if (memcmp(name, PREFIX_LOGICAL, strlen(PREFIX_LOGICAL)) == 0)
    {
        prefix = PREFIX_LOGICAL;
    }

    return prefix;
}

int PartitionContext::
getType(const char* name)
{
    int type = -1;
    if (memcmp(name, PREFIX_PRIMARY, strlen(PREFIX_PRIMARY)) == 0)
    {
        type = TYPE_PRIMARY;
    }
    else if (memcmp(name, PREFIX_EXTENDED, strlen(PREFIX_EXTENDED)) == 0)
    {
        type = TYPE_EXTENDED;
    }
    else if (memcmp(name, PREFIX_LOGICAL, strlen(PREFIX_LOGICAL)) == 0)
    {
        type = TYPE_LOGICAL;
    }

    return type;
}

PartitionStream* PartitionContext::
lookupPartitionStream(u8 type, u8 id)
{
    Monitor::Synchronized method(monitor);

    PartitionStreamList::Iterator iter = partitionList.begin();
    PartitionStream* stream;
    while (stream = iter.next())
    {
        if (stream->getId() == id)
        {
            if ((type == TYPE_PRIMARY && stream->isPrimaryPartition()) ||
                (type == TYPE_EXTENDED && stream->isExtendedPartition()) ||
                (type == TYPE_LOGICAL && stream->isLogicalPartition()))
            {
                return stream;
            }
        }
    }

    return 0;
}

PartitionStream* PartitionContext::
lookupPartitionStream(const char* name)
{
    int n = getId(name, getPrefix(name));
    int type = getType(name);

    if (type < 0 || n < 0 || 255 < n)
    {
        return 0;
    }

    return lookupPartitionStream(type, n);
}

u8* PartitionContext::
getEntry(u8* mbr, u8 type, u8 number)
{
    Monitor::Synchronized method(monitor);

    u8* entry = &mbr[MBR_PartitionTable];

    if (type == TYPE_LOGICAL)
    {
        return entry;
    }

    int entryNo = 0;
    if (type == TYPE_EMPTY)
    {
        int i;
        for (i = 0; i < 4; ++i, entry += MBR_EntrySize)
        {
            u8 system = byte(entry + MBR_SystemIndicator);
            if (!system)
            {
                if (entryNo == number)
                {
                    return entry;
                }
                ++entryNo;
            }
        }
        return 0;
    }

    PartitionStream* stream = lookupPartitionStream(type, number);
    if (!stream)
    {
        return 0;
    }

    return stream->getEntry(mbr);
}

int PartitionContext::
clearBootRecord(PartitionStream* stream)
{
    PartitionStream::Partition partition;
    partition.startingOffset = 0;
    partition.partitionLength = 0;
    partition.hiddenSectors = 0;
    partition.partitionType = 0;
    partition.bootIndicator = 0;

    try
    {
        stream->setLayout(&partition);
    }
    catch (Exception& error)
    {
        return -1;
    }

    return 0;
}

int PartitionContext::
clearParentBootRecord(PartitionStream* stream)
{
    PartitionStream::Partition partition;
    partition.startingOffset = 0;
    partition.partitionLength = 0;
    partition.hiddenSectors = 0;
    partition.partitionType = 0;
    partition.bootIndicator = 0;
    if (stream->setParentEPBR(&partition) < 0)
    {
        return -1;
    }
    return 0;
}

int PartitionContext::
removePartition(u8 type, u8 id)
{
    PartitionStream* stream = lookupPartitionStream(type, id);
    if (!stream || !stream->isRemovable())
    {
        return -1;
    }

    if (type == TYPE_EXTENDED || type == TYPE_LOGICAL)
    {
        if (stream != partitionList.getLast())
        {
            return -1;
        }
    }

    if (clearBootRecord(stream) < 0)
    {
        return -1;
    }

    if (type == TYPE_LOGICAL &&
        clearParentBootRecord(stream) < 0)
    {
        return -1;
    }

    partitionList.remove(stream);
    stream->release();
    delete stream;

    return 0;
}

int PartitionContext::
checkPartition(PartitionStream* stream, PartitionStream::Geometry* geometry,
    PartitionStream::Partition* partition)
{
    if (geometry->diskSize <= partition->startingOffset + partition->partitionLength)
    {
        return -1;
    }

    if (partition->partitionLength % geometry->bytesPerSector ||
        partition->hiddenSectors != 0)
    {
        return -1;
    }

    if (partition->bootIndicator != BOOT_FLAG_INACTIVE &&
        partition->bootIndicator != BOOT_FLAG_ACTIVE)
    {
        return -1;
    }

    // check if the partition overlaps the next partition.
    u8 entryNo = stream->getEntryNo();
    PartitionStream* next = 0;
    if (stream->isLogicalPartition())
    {
        next = lookupPartitionStream(TYPE_LOGICAL, stream->getId() + 1);
    }
    else if (entryNo < 3)
    {
        Monitor::Synchronized method(monitor);

        PartitionStreamList::Iterator iter = partitionList.begin();
        while (next = iter.next())
        {
            if (next->getEntryNo() == entryNo + 1)
            {
                break;
            }
        }
    }

    if (next)
    {
        PartitionStream::Partition nextPartition;
        next->getLayout(&nextPartition);

        if  (nextPartition.startingOffset <=
            partition->startingOffset + partition->partitionLength)
        {
            return -1;
        }
    }

    return 0;
}

u8 PartitionContext::
getDefaultPartitionType(long long size)
{
    if (size <= 4 * 1024 * 1024LL)
    {
        return PT_FAT12;
    }
    else if (size < 32 * 1024 * 1024LL)
    {
        return PT_FAT16_UPTO_32MB;
    }
    else if (size < 512 * 1024 * 1024LL)
    {
        return PT_FAT16_OVER_32MB;
    }
    else
    {
        return PT_FAT32;
    }
}

//
// PartitionContext : es::Partition
//

int PartitionContext::
initialize()
{
    if (!disk)
    {
        return -1;
    }

    u8* sector = new u8[512];
    if (!sector)
    {
        return -1;
    }
    memmove(sector, mbr, 512);

    disk->write(sector, 512, 0);
    disk->flush();

    delete[] sector;

    return 0;
}

/*
 *  In the partition list,
 *  (1) the extended partition must be included before logical partition.
 *  (2) Logical partitions must be listed in numerical order.
 *  (3) Primary partitions must be included before
 *      the extended partition (in no particular order).
 *
 *  (first) partition0 - partition2 - partition1 - extended -
 *          logical0 - logical1 - ... - logicalN (last)
 *
 *  The order is important when partitions are unmounted.
 */
int PartitionContext::
mount(es::Stream* disk)
{
    Monitor::Synchronized method(monitor);

    this->disk = disk;

    es::DiskManagement::Geometry geometry;
    getGeometry(&geometry);

    // Read MBR
    u8 mbr[geometry.bytesPerSector];
    disk->read(mbr, geometry.bytesPerSector, 0);
    if (word(mbr + MBR_Signature) != MBR_SIGNATURE)
    {
        esReport("Not found MBR.\n");
        return -1;
    }

    int id = 0;
    int i;
    PartitionStream* parent;
    PartitionStream* stream;
    u8* entry = &mbr[MBR_PartitionTable];
    u8* extended = 0;
    for (i = 0; i < 4; ++i, entry += MBR_EntrySize)
    {
#ifdef VERBOSE
        // check entry
        esReport("%02x %06x %02x %06x %08x %08x\n",
                 byte(entry + MBR_BootIndicator),
                 dword(entry + MBR_StartingCHS) & 0xffffff,
                 byte(entry + MBR_SystemIndicator),
                 dword(entry + MBR_EndingCHS) & 0xffffff,
                 dword(entry + MBR_StartingSector),
                 dword(entry + MBR_TotalSectors));
#endif // VERBOSE
        switch (byte(entry + MBR_SystemIndicator))
        {
        case PT_EMPTY:
            break;
        case PT_EXTENDED:
        case PT_EXTENDED_LBA:
            // Extended partition
            if (extended)
            {
                // A disk may contain one extended partition.
                break;
            }
            extended = entry;
            stream = new PartitionStream(this, 0, entry, (u8) i);
            partitionList.addLast(stream);
            parent = stream;
            break;
        default:
            stream = new PartitionStream(this, id, entry, (u8) i);
            partitionList.addFirst(stream);
            ++id;
            break;
        }
    }
    esReport("\n");

    if (!extended)
    {
        return 0;
    }

    id = 0;
    u32 base = dword(extended + MBR_StartingSector);
    u32 epbr = base; // Extended Partition Boot Record
    while (extended)
    {
        disk->read(mbr, geometry.bytesPerSector, (long long) epbr * geometry.bytesPerSector);
        if (word(mbr + MBR_Signature) != MBR_SIGNATURE)
        {
            break;
        }

        entry = &mbr[MBR_PartitionTable];

#ifdef VERBOSE
        // check entry
        esReport("%02x %06x %02x %06x %08x %08x %08x\n",
                 byte(entry + MBR_BootIndicator),
                 dword(entry + MBR_StartingCHS) & 0xffffff,
                 byte(entry + MBR_SystemIndicator),
                 dword(entry + MBR_EndingCHS) & 0xffffff,
                 dword(entry + MBR_StartingSector),
                 dword(entry + MBR_TotalSectors),
                 epbr +  dword(entry + MBR_StartingSector));
#endif // VERBOSE
        switch (byte(entry + MBR_SystemIndicator))
        {
        case PT_EMPTY:
        case PT_EXTENDED:
        case PT_EXTENDED_LBA:
            break;
        default:
            stream = new PartitionStream(this, id, entry, 0, epbr, parent);
            partitionList.addLast(stream);
            parent = stream;
            break;
        }
        extended = &mbr[MBR_PartitionTable + 16];
        if (byte(extended + MBR_SystemIndicator) != PT_EXTENDED)
        {
            extended = 0;
        }
        else
        {
            epbr = base + dword(extended + MBR_StartingSector);
        }

        ++id;
    }

    return 0;
}

int PartitionContext::
unmount()
{
    Monitor::Synchronized method(monitor);

    PartitionStreamList::Iterator iter = partitionList.begin();
    PartitionStream* stream;
    PartitionStream* last = 0;

    while (stream = iter.next())
    {
        if (!stream->isRemovable())
        {
            // The partition is busy.
            return -1;
        }
    }

    while (!partitionList.isEmpty())
    {
        // remove partitions in a reverse order.
        PartitionStream* stream = partitionList.removeLast();
        stream->release();
        delete stream;
    }

    this->disk = 0;
    return 0;
}

//
// PartitionContext : es::Context
//

es::Binding* PartitionContext::
bind(const char* name, Object* object)
{
    Monitor::Synchronized method(monitor);

    if (object || !disk)
    {
        return 0;
    }

    PartitionStream* ps = lookupPartitionStream(name);
    if (!ps)
    {
        u8 type = getType(name);
        switch (type)
        {
          case TYPE_PRIMARY:
          case TYPE_EXTENDED:
            ps = createPartition(name, type);
            break;
          case TYPE_LOGICAL:
            ps = createLogicalPartition(name);
            break;
          default:
            return 0;
        }

        if (!ps)
        {
            return 0;
        }
    }

    Handle<es::Stream> created(ps, true);
    Handle<es::Iterator> iter = list("");
    Handle<es::Binding> binding;
    while ((binding = iter->next()))
    {
        Handle<es::Stream> stream = binding->getObject();
        if (stream == created)
        {
            es::Binding* ret = binding;
            ret->addRef();
            return ret;
        }
    }

    return 0;
}

es::Context* PartitionContext::
createSubcontext(const char* name)
{
    return 0;
}

int PartitionContext::
destroySubcontext(const char* name)
{
    return -1;
}

int PartitionContext::
getId(const char* name, const char* prefix)
{
    if (!name || !prefix)
    {
        return -1;
    }

    const char* p = prefix;
    while (*p)
    {
        if (*p++ != *name++)
        {
            return -1;
        }
    }

    int n;
    for (n = 0; *name; ++name)
    {
        if (!isdigit(*name))
        {
            return -1;
        }
        if (n == 0 && *name == '0' && *(name + 1) == 0)
        {
            return 0;
        }
        n = 10 * n + (*name - '0');
    }

    return n;
}

Object* PartitionContext::
lookup(const char* name)
{
    PartitionStream* stream = lookupPartitionStream(name);
    if (!stream)
    {
        return 0;
    }

    stream->addRef();
    return static_cast<es::Stream*>(stream);
}

int PartitionContext::
rename(const char* oldName, const char* newName)
{
    return -1;
}

int PartitionContext::
unbind(const char* name)
{
    Monitor::Synchronized method(monitor);

    if (!disk)
    {
        return 0;
    }

    int type = getType(name);
    int id = getId(name, getPrefix(name));
    if (type < 0 || id < 0 || 255 < id)
    {
        return -1;
    }

    switch (type)
    {
      case TYPE_PRIMARY:
      case TYPE_EXTENDED:
      case TYPE_LOGICAL:
        return removePartition(type, id);
    }

    return -1;
}

es::Iterator* PartitionContext::
list(const char* name)
{
    return new PartitionIterator(this);
}

//
// PartitionContext : Object
//

Object* PartitionContext::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Context::iid()) == 0)
    {
        objectPtr = static_cast<es::Context*>(this);
    }
    else if (strcmp(riid, es::Partition::iid()) == 0)
    {
        objectPtr = static_cast<es::Partition*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Context*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int PartitionContext::
addRef()
{
    return ref.addRef();
}

unsigned int PartitionContext::
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


es::Partition* PartitionContext::
Constructor::createInstance()
{
    return new PartitionContext;
}

Object* PartitionContext::
Constructor::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Partition::Constructor::iid()) == 0)
    {
        objectPtr = static_cast<es::Partition::Constructor*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Partition::Constructor*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int PartitionContext::
Constructor::addRef()
{
    return 1;
}

unsigned int PartitionContext::
Constructor::release()
{
    return 1;
}

void PartitionContext::
initializeConstructor()
{
    static Constructor constructor;
    es::Partition::setConstructor(&constructor);
}