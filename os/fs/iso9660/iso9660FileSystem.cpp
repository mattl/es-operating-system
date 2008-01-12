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

#include <errno.h>
#include <string.h>
#include <es.h>
#include <es/classFactory.h>
#include <es/handle.h>
#include "iso9660Stream.h"

const DateTime Iso9660FileSystem::
epoch(1900, 1, 1);

const char* Iso9660FileSystem::
ucs2EscapeSequences[3] =
{
    "%/@",  // Level 1
    "%/C",  // Level 2
    "%/E",  // Level 3
};

int Iso9660FileSystem::
hashCode(u32 dirLocation, u32 offset)
{
    return (int) (dirLocation + offset);
}

void Iso9660FileSystem::
init()
{
    hashSize = 20;
    hashTable = new Iso9660StreamChain[hashSize];

    esCreateInstance(CLSID_CacheFactory,
                     IID_ICacheFactory,
                     reinterpret_cast<void**>(&cacheFactory));
}

Iso9660FileSystem::
Iso9660FileSystem() :
    disk(0)
{
    init();
}

Iso9660FileSystem::
Iso9660FileSystem(IStream* disk) :
    disk(0)
{
    init();
    mount(disk);
}

Iso9660FileSystem::
~Iso9660FileSystem()
{
    dismount();
    cacheFactory->release();
    delete[] hashTable;
}

// The reference count of the looked up stream shall be incremented by one.
Iso9660Stream* Iso9660FileSystem::
lookup(u32 dirLocation, u32 offset)
{
    Iso9660Stream* stream;
    Iso9660StreamChain::Iterator iter =
        hashTable[hashCode(dirLocation, offset) % hashSize].begin();
    while ((stream = iter.next()))
    {
        if (stream->dirLocation == dirLocation && stream->offset == offset)
        {
            stream->addRef();
            return stream;
        }
    }
    return 0;
}

void Iso9660FileSystem::
add(Iso9660Stream* stream)
{
    {
        hashTable[stream->hashCode() % hashSize].addFirst(stream);
    }
}

void Iso9660FileSystem::
remove(Iso9660Stream* stream)
{
    if (!stream->isRoot())
    {
        hashTable[stream->hashCode() % hashSize].remove(stream);
    }
}

DateTime Iso9660FileSystem::
getTime(u8* dt)
{
    try
    {
        return DateTime(1900 + dt[DT_Year], dt[DT_Month], dt[DT_Day],
                        dt[DT_Hour], dt[DT_Minute], dt[DT_Second]);
    }
    catch (...)
    {
        return epoch;
    }
}

Iso9660Stream* Iso9660FileSystem::
getRoot()
{
    root->addRef();
    return root;
}

Iso9660Stream* Iso9660FileSystem::
createStream(Iso9660FileSystem* fileSystem, Iso9660Stream* parent, u32 offset, u8* record)
{
    if (escapeSequences == 0)
    {
        return new Iso9660Stream(fileSystem, parent, offset, record);
    }
    else
    {
        return new Iso9660StreamUcs2(fileSystem, parent, offset, record);
    }
}

bool Iso9660FileSystem::
isDelimitor(int c)
{
    return (c == '/' || c == '\\') ? true : false;
}

void Iso9660FileSystem::
mount(IStream* disk)
{
    if (!disk)
    {
        return;
    }

    if (this->disk)
    {
        esThrow(EALREADY);
    }

    this->disk = disk;
    disk->addRef();

    bytsPerSec = 2048;
    escapeSequences = 0;

    diskCache = cacheFactory->create(disk);
    diskCache->setSectorSize(2048);

    Handle<IStream> stream(diskCache->getInputStream());
    stream->setPosition(16 * bytsPerSec);

    u8 vd[2048];
    bool done = false;
    while (!done)
    {
        if (stream->read(vd, sizeof vd) < sizeof vd)
        {
            root = 0;
            return;
        }

        switch (vd[VD_Type])
        {
        case VDT_BootRecord:
            esReport("[Boot Record]\n");
            esReport("Standard Identifier: %.5s\n", &vd[VD_StandardIdentifier]);
            esReport("Version: %d\n", vd[VD_Version]);
            break;
        case VDT_Primary:
            esReport("[Primary]\n");
            esReport("Standard Identifier: %.5s\n", &vd[VD_StandardIdentifier]);
            esReport("Version: %d\n", vd[VD_Version]);

            esReport("System Identifier: %.32s\n", &vd[VD_SystemIdentifier]);
            esReport("Volume Identifier: %.32s\n", &vd[VD_VolumeIdentifier]);
            esReport("Volume Space Size: %d\n", LittleEndian::dword(vd + VD_VolumeSpaceSize));
            esReport("Volume Space Size: %d\n", BigEndian::dword(vd + VD_VolumeSpaceSize + 4));

            esReport("Volume Set Size: %d\n", LittleEndian::word(vd + VD_VolumeSetSize));
            esReport("Volume Set Size: %d\n", BigEndian::word(vd + VD_VolumeSetSize + 2));
            esReport("Volume Sequence Number: %d\n", LittleEndian::word(vd + VD_VolumeSequenceNumber));
            esReport("Volume Sequence Number: %d\n", BigEndian::word(vd + VD_VolumeSequenceNumber + 2));
            esReport("Logical Block Size: %d\n", LittleEndian::word(vd + VD_LogicalBlockSize));
            esReport("Logical Block Size: %d\n", BigEndian::word(vd + VD_LogicalBlockSize + 2));

            esReport("Path Table Size: %d\n", LittleEndian::dword(vd + VD_PathTableSize));

            if (!escapeSequences)
            {
                memmove(rootRecord, vd + VD_RootDirectory, vd[VD_RootDirectory + DR_Length]);
            }
            break;
        case VDT_Supplementary:
            esReport("[Supplementary]\n");
            esReport("Standard Identifier: %.5s\n", &vd[VD_StandardIdentifier]);
            esReport("Version: %d\n", vd[VD_Version]);

            esReport("System Identifier: %.32s\n", &vd[VD_SystemIdentifier]);
            esReport("Volume Identifier: %.32s\n", &vd[VD_VolumeIdentifier]);
            esReport("Volume Space Size: %d\n", LittleEndian::dword(vd + VD_VolumeSpaceSize));
            esReport("Volume Space Size: %d\n", BigEndian::dword(vd + VD_VolumeSpaceSize + 4));

            esReport("Volume Set Size: %d\n", LittleEndian::word(vd + VD_VolumeSetSize));
            esReport("Volume Set Size: %d\n", BigEndian::word(vd + VD_VolumeSetSize + 2));
            esReport("Volume Sequence Number: %d\n", LittleEndian::word(vd + VD_VolumeSequenceNumber));
            esReport("Volume Sequence Number: %d\n", BigEndian::word(vd + VD_VolumeSequenceNumber + 2));
            esReport("Logical Block Size: %d\n", LittleEndian::word(vd + VD_LogicalBlockSize));
            esReport("Logical Block Size: %d\n", BigEndian::word(vd + VD_LogicalBlockSize + 2));

            esReport("Path Table Size: %d\n", LittleEndian::dword(vd + VD_PathTableSize));

            esReport("Escape Sequences: %s\n", vd + VD_EscapeSequences);

            const char** seq;
            for (seq = &ucs2EscapeSequences[0];
                 seq < &ucs2EscapeSequences[3];
                 ++seq)
            {
                if (strcmp(*seq, (const char*) vd + VD_EscapeSequences) == 0)
                {
                    escapeSequences = *seq;
                    memmove(rootRecord, vd + VD_RootDirectory, vd[VD_RootDirectory + DR_Length]);
                    esReport("ISO 9660SVD - Unicode (UCS-2): %s\n", escapeSequences);
                    break;
                }
            }
            break;
        case VDT_Terminator:
            esReport("[Terminator]\n");
            esReport("Standard Identifier: %.5s\n", &vd[VD_StandardIdentifier]);
            esReport("Version: %d\n", vd[VD_Version]);
            done = true;
            break;
        default:
            esReport("[Unknown %d]\n", vd[VD_Type]);
            break;
        }
    }

    // Create the root node
    root = createStream(this, 0, 0, rootRecord);
}

void Iso9660FileSystem::
dismount(void)
{
    if (!disk)
    {
        return;
    }

    if (root)
    {
        root->flush();
        root->release();
        root = 0;

        diskCache->release();
    }

    disk->release();
    disk = 0;
    bytsPerSec = 0;
}

void Iso9660FileSystem::
getRoot(IContext** root)
{
    if (root)
    {
        *root = getRoot();
    }
}

void Iso9660FileSystem::
getFreeSpace(long long& freeBytes)
{
    freeBytes = 0;
}

void Iso9660FileSystem::
getTotalSpace(long long& bytes)
{
    bytes = 0;  // XXX
}

int Iso9660FileSystem::
checkDisk(bool fixError)
{
    return 0;
}

void Iso9660FileSystem::
format()
{
}

int Iso9660FileSystem::
defrag()
{
    return 0;
}

bool Iso9660FileSystem::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IFileSystem)
    {
        *objectPtr = static_cast<IFileSystem*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IFileSystem*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Iso9660FileSystem::
addRef(void)
{
    return ref.addRef();
}

unsigned int Iso9660FileSystem::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}

void esRegisterIsoFileSystemClass(IClassStore* classStore)
{
    // Register CLSID_MonitorFactory
    IClassFactory* isoFileSystemFactory = new(ClassFactory<Iso9660FileSystem>);
    classStore->add(CLSID_IsoFileSystem, isoFileSystemFactory);
}
