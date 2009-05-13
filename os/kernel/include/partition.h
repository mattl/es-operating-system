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

#include <es/types.h>
#include <es/list.h>
#include <es/endian.h>
#include <es/ref.h>
#include <es/base/IStream.h>
#include <es/device/IDiskManagement.h>
#include <es/device/IPartition.h>
#include <es/util/IIterator.h>
#include <es/naming/IBinding.h>
#include <es/naming/IContext.h>
#include "thread.h"

class PartitionContext;
class PartitionIterator;

class PartitionStream : public es::Stream, public es::DiskManagement
{
    Ref                     ref;
    PartitionContext*       context;
    u8                      boot;
    u8                      system;
    long long               offset;
    long long               size;
    long long               br; // MBR or EPBR
    u8                      entryNo;

    u8                      type; // primary, extended or logical;
    u8                      id;
    PartitionStream*        parent;

public:
    Link<PartitionStream>   link;

    PartitionStream(PartitionContext* context, int id,
        u8* entry, u8 entryNo, u32 base = 0, PartitionStream* parent = 0);
    ~PartitionStream();

    bool isPrimaryPartition();
    bool isExtendedPartition();
    bool isLogicalPartition();
    bool isRemovable();
    int setEntry(u8* entry, Partition* partition, Geometry* geometry, bool secondEntry = false);
    int setParentEPBR(Partition* partition);
    void adjustSize(Geometry* geometry, Partition* partition, long long& partitionSize);
    void getOffset(long long& offset);
    u8 getId();
    u8 getType();
    u8* getEntry(u8* mbr);
    void setType(u8 type);
    u8 getEntryNo();

    // IStream
    long long getPosition();
    void setPosition(long long pos);
    long long getSize();
    void setSize(long long size);
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();

    // IDiskManagement
    int initialize();
    void getGeometry(Geometry* geometry);
    void getLayout(Partition* partition);
    void setLayout(const Partition* partition);

    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

class PartitionContext : public es::Partition
{
    friend class PartitionStream;
    friend class PartitionIterator;

    typedef List<PartitionStream, &PartitionStream::link>   PartitionStreamList;

    Ref                 ref;
    Monitor             monitor;
    es::Stream*         disk;
    PartitionStreamList partitionList;

    PartitionStream* createPartition(const char* name, u8 type);
    PartitionStream* createLogicalPartition(const char* name);
    int initEPBR(u8* buf, u32 len, long long offset);
    int getEPBROffset(PartitionStream* last, long long& offset);
    PartitionStream* lookupPartitionStream(u8 type, u8 number);
    PartitionStream* lookupPartitionStream(const char* name);
    int getId(const char* name, const char* prefix);
    int getGeometry(es::DiskManagement::Geometry* geometry);
    const char* getPrefix(const char* name);
    int getType(const char* name);
    int clearBootRecord(PartitionStream* stream);
    int clearParentBootRecord(PartitionStream* stream);
    int removePartition(u8 type, u8 id);
    u8 getDefaultPartitionType(long long size);

    static const unsigned MAX_CYLINDER = 1023;
    static const unsigned HEADS_PER_CYLINDER = 255;
    static const unsigned SECTORS_PER_HEAD = 63;
    static const unsigned SECTOR_OFFSET = 1;
    static const unsigned MAX_CHS = (MAX_CYLINDER * HEADS_PER_CYLINDER + HEADS_PER_CYLINDER - 1)
                                    * SECTORS_PER_HEAD + SECTORS_PER_HEAD - 1;

    static const unsigned MAX_STARTING_CHS = 0xFFC101;
    static const unsigned MAX_STARTING_EPBR_CHS = 0xFFC100;
    static const unsigned MAX_ENDING_CHS = 0xFFFFFE;

public:
    PartitionContext();
    ~PartitionContext();

    int convertLBAtoCHS(unsigned startingSector, unsigned totalSectors,
            unsigned& startingCHS, unsigned& endingCHS);
    u8* getEntry(u8* mbr, u8 type, u8 id);
    int checkPartition(PartitionStream* stream, PartitionStream::Geometry* geometry,
        PartitionStream::Partition* partition);

    // IContext
    es::Binding* bind(const char* name, Object* object);
    es::Context* createSubcontext(const char* name);
    int destroySubcontext(const char* name);
    Object* lookup(const char* name);
    int rename(const char* oldName, const char* newName);
    int unbind(const char* name);
    es::Iterator* list(const char* name);

    // IPartition
    int initialize();
    int mount(es::Stream* disk);
    int unmount();

    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    static const char* PREFIX_PRIMARY;
    static const char* PREFIX_EXTENDED;
    static const char* PREFIX_LOGICAL;
    static const int MAX_PREFIX_LEN;

    static const unsigned MBR_SIGNATURE = 0xaa55;
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

    enum
    {
        TYPE_EMPTY,
        TYPE_PRIMARY,
        TYPE_EXTENDED,
        TYPE_LOGICAL
    };

    enum // partition type
    {
        PT_EMPTY           = 0x00,
        PT_FAT12           = 0x01,
        PT_FAT16_UPTO_32MB = 0x04,
        PT_EXTENDED        = 0x05,
        PT_FAT16_OVER_32MB = 0x06,
        PT_FAT32           = 0x0B,
        PT_FAT32_LBA       = 0x0C,
        PT_FAT16_LBA       = 0x0E,
        PT_EXTENDED_LBA    = 0x0F
    };

    enum
    {
        BOOT_FLAG_INACTIVE = 0x00,
        BOOT_FLAG_ACTIVE   = 0x80
    };

    // [Constructor]
    class Constructor : public es::Partition::Constructor
    {
    public:
        es::Partition* createInstance();
        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();
    };

    static void initializeConstructor();
};

class PartitionIterator : public es::Iterator, public es::Binding
{
    typedef List<PartitionStream, &PartitionStream::link>   PartitionStreamList;

    Ref                 ref;
    PartitionContext*   context;
    long long           ipos;

public:
    PartitionIterator(PartitionContext* context, int ipos = 0);
    ~PartitionIterator();

    // IIterator
    bool hasNext();
    Object* next();
    int remove();

    // IBinding
    Object* getObject();
    void setObject(Object* object);
    const char* getName(void* name, int len);

    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};
