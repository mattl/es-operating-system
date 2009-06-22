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

// FAT File Subsystem

#include <es/types.h>
#include <es/list.h>
#include <es/dateTime.h>
#include <es/endian.h>
#include <es/ref.h>
#include <es/synchronized.h>
#include <es/utf.h>
#include <es/base/IMonitor.h>
#include <es/base/ICache.h>
#include <es/base/IFile.h>
#include <es/base/IStream.h>
#include <es/base/IPageable.h>
#include <es/util/IIterator.h>
#include <es/naming/IBinding.h>
#include <es/naming/IContext.h>
#include <es/device/IDisk.h>
#include <es/device/IFatFileSystem.h>
#include "fat.h"

using namespace LittleEndian;

class FatStream;
class FatFileSystem;
class FatIterator;

class FatStream : public es::File, public es::Stream, public es::Context, public es::Binding
{
    friend class FatFileSystem;
    friend class FatIterator;

    enum
    {
        Removed = 0x01,
        Updated = 0x02
    };

    Ref             ref;
    FatFileSystem*  fileSystem;
    Link<FatStream> linkChain;
    Link<FatStream> linkHash;

    es::Monitor*    monitor;
    es::Cache*      cache;
    FatStream*      parent;

    u32         dirClus;    // the first cluster of the parent directory. Zero if this node is the root.
    u32         offset;     // offset to the directory entry of this node.
    u32         fstClus;
    u32         size;

    u8          fcb[32];
    u32         flags;

    // getClusNum() support
    long long   lastPosition;
    u32         lastClus;

public:
    FatStream(FatFileSystem* fileSystem, FatStream* parent, u32 offset, u8* fcb);
    ~FatStream();

    // fatContext.cpp
    static bool findNext(es::Stream* dir, u8* fcb, u16* fileName,
                         int freeRequired, int& freeOffset, u32& freeSize);
    static bool findNext(es::Stream* dir, u8* fcb, u16* fileName);
    static FatStream* lookup(FatStream* stream, const char*& name);
    bool isEmpty();
    bool isRoot();
    FatStream* create(const char* name, u8 attr);
    int remove();
    int hashCode() const;

    // es::File
    unsigned int getAttributes();
    long long getCreationTime();
    long long getLastAccessTime();
    long long getLastWriteTime();
    void setAttributes(unsigned int attributes);
    void setCreationTime(long long time);
    void setLastAccessTime(long long time);
    void setLastWriteTime(long long time);
    bool canRead();
    bool canWrite();
    bool isDirectory();
    bool isFile();
    bool isHidden();
    es::Stream* getStream();
    es::Pageable* getPageable();

    // es::Stream
    long long getPosition();
    void setPosition(long long pos);
    long long getSize();
    void setSize(long long size);
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();

    // es::Binding
    Object* getObject();
    void setObject(Object* object);
    const char* getName(void* name, int len);

    // es::Context
    es::Binding* bind(const char* name, Object* object);
    es::Context* createSubcontext(const char* name);
    int destroySubcontext(const char* name);
    Object* lookup(const char* name);
    int rename(const char* oldName, const char* newName);
    int unbind(const char* name);
    es::Iterator* list(const char* name);

    // Object
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

private:
    u32 getClusNum(long long position);
    bool check(u8* clusRefs);

    // fatTime.cpp
    void setCreationTime(DateTime);
    void setLastAccessTime(DateTime);
    void setLastWriteTime(DateTime);

    bool isRemoved();
};

class FatFileSystem : public es::FatFileSystem
{
    typedef List<FatStream, &FatStream::linkHash>   FatStreamChain;
    typedef List<FatStream, &FatStream::linkChain>  FatStreamList;
    friend class FatStream;
    friend class PartitionStream;

    struct Geometry
    {
        unsigned int heads;
        unsigned int cylinders;
        unsigned int sectorsPerTrack;
        unsigned int bytesPerSector;
        long long diskSize;
    };

    Ref             ref;
    es::Stream*     partition;
    es::PageSet*    pageSet;
    es::Cache*      diskCache;
    es::Stream*     diskStream;
    FatStream*      root;

    es::Monitor*    hashMonitor;    // monitor for the hash table and standby list
    size_t          hashSize;
    FatStreamChain* hashTable;
    FatStreamList   standbyList;

    es::Monitor*    fatMonitor;     // monitor for FAT

    // bpb
    u8      bpb[512];
    u8      fsi[512];

    u16     bytsPerSec;
    u32     bytsPerClus;
    u32     hiddSec;

    u32     firstRootDirSector;
    u32     rootDirSectors;
    u32     fatSz;
    u32     totSec;
    u32     firstDataSector;
    u32     dataSec;
    u32     countOfClusters;

    u32     nxtFree;
    u32     freeCount;

    void init();

    u32 firstSectorOfCluster(u32 n);
    bool isEof(u32 clus);
    bool isBadCluster(u32 clus);

    // fatCluster.cpp
    u32 calcSize(u32 clus);
    int readCluster(void* dst, int count, u32 clus, int offset);
    int writeCluster(const void* src, int count, u32 clus, int offset);
    int zeroCluster(u32 clus);
    u32  allocCluster(u32 n = 1, bool zero = false);
    void freeCluster(u32 clus);
    u32  clusEntryVal(u32 n);
    void setClusEntryVal(u32 n, u32 v);

    static u8* zero;                // Zero cleared region for DMA

public:
    const static u8 nameDot[11];
    const static u8 nameDotdot[11];

    const static DateTime epoch;    // 01/01/1980
    const static DateTime wane;     // 12/31/2107

    // fat.cpp
    FatFileSystem();
    FatFileSystem(es::Stream* partition);
    ~FatFileSystem();
    FatStream* getRoot();
    FatStream* lookup(u32 dirClus, u32 offset);
    void add(FatStream* stream);
    void remove(FatStream* stream);
    void activate(FatStream* stream);
    void standBy(FatStream* stream);
    bool isClean();
    void setClean(bool clean);

    // fatCheck.cpp
    bool check();

    // fatUtf.cpp
    static bool isDelimitor(int c);
    static bool isValidShortChar(u8 ch);
    static bool isValidLongChar(u8 ch);
    static bool oemtoutf16(const u8* oem, u16* utf16);
    static u8 oemCode(u16 utf16, bool& lossy);
    static bool utf16tooem(const u16* utf16, u8* oem);
    static void utf16toutf8(const u16* utf16, char* utf8);
    static void utf8toutf16(const char* utf8, u16* utf16);
    static const char* splitPath(const char* utf8, u16* utf16);
    static bool isEqual(const u16* fileName, const u16* fcbName, const u8* fcb);
    static u16* assembleLongName(u16* longName, u8* fcb);
    static void fillLongName(u8* fcb, const u16* longName, int ord);
    static int getNumericTrail(const u8*& oem);
    static int getNumericTrail(const u8* oem, const u8* fcb);
    static void setNumericTrail(u8* oem, int n);

    // fat.cpp
    static bool isFatPartition(u8 type);
    bool isFat12();
    bool isFat16();
    bool isFat32();
    static u8 getChecksum(u8* fcb);
    static bool isFreeEntry(u8* fcb);
    static bool canRead(u8* fcb);
    static bool canWrite(u8* fcb);
    static bool isDirectory(u8* fcb);
    static bool isFile(u8* fcb);
    static bool isHidden(u8* fcb);
    static bool isLongNameComponent(u8* fcb);
    static bool isVolumeID(u8* fcb);
    static int hashCode(u32 dirClus, u32 offset);

    // fatFormat.cpp
    static int format(es::Stream* partition);
    static int formatFat12(es::Stream* partition);
    static int formatFat16(es::Stream* partition);
    static int formatFat32(es::Stream* partition);
    static void getGeometry(es::Stream* partition, Geometry* geometry);
    int updateBootCode();

    // es::FileSystem
    void mount(es::Stream* disk);
    void dismount(void);
    void getRoot(es::Context** root);
    long long getFreeSpace();
    long long getTotalSpace();
    int checkDisk(bool fixError);
    void format();
    int defrag();

    // Object
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    // [Constructor]
    class Constructor : public es::FatFileSystem::Constructor
    {
    public:
        es::FatFileSystem* createInstance();
        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();
    };

    static void initializeConstructor();
};

class FatIterator : public es::Iterator
{
    Ref         ref;
    FatStream*  stream;
    long long   ipos;

public:

    FatIterator(FatStream* stream);
    ~FatIterator();

    bool hasNext(void);
    Object* next();
    int remove(void);

    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};
