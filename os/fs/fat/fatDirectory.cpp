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

// FAT Directory operationss

#include <ctype.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include "fatStream.h"

extern "C"
{
    int ffs(int param);
}

int FatStream::
hashCode() const
{
    return FatFileSystem::hashCode(dirClus, offset);
}

bool FatStream::
isRoot()
{
    return parent ? false : true;
}

bool FatStream::
isEmpty()
{
    ASSERT(isDirectory());

    u8 ent[32];
    u16 longName[256];
    Handle<es::Stream> dir(cache->getStream());
    if (!isRoot())
    {
        dir->setPosition(2 * 32);    // Skip dot and dotdot.
    }
    return !findNext(dir, ent, longName);
}

bool FatStream::
isRemoved()
{
    return (flags & Removed) ? true : false;
}

bool FatStream::
findNext(es::Stream* dir, u8* ent, u16* fileName,
         int freeRequired, int& freeOffset, u32& freeSize)
{
    int ord = -1;   // The order of long-name entry
    u8 sum;
    u16 longName[13 * 20 + 1];
    u16* l;
    long long pos;

    pos = dir->getPosition();
    freeOffset = 0;
    freeSize = 0;
    while (dir->read(ent, 32) == 32)
    {
        if (ord < 0)
        {
            l = &longName[13 * 20 + 1];
            *--l = 0;
        }
        pos = dir->getPosition();
        if (FatFileSystem::isFreeEntry(ent))
        {
            if (freeOffset + freeSize == pos - 32)
            {
                freeSize += 32;
            }
            else if (freeSize < freeRequired)
            {
                freeOffset = pos - 32;
                freeSize = 32;
            }
            ord = -1;
            if (ent[0] == 0x00)
            {
                break;
            }
        }
        else if (FatFileSystem::isLongNameComponent(ent))
        {
            if (ent[LDIR_Ord] & LAST_LONG_ENTRY)
            {
                ord = ent[LDIR_Ord] & ~LAST_LONG_ENTRY;
                if (0 < ord && ord <= 20)
                {
                    sum = ent[LDIR_Chksum];
                    l = FatFileSystem::assembleLongName(l, ent);
                    --ord;
                }
                else
                {
                    ord = -1;   // Orphan
                }
            }
            else if (0 < ord && ord == ent[LDIR_Ord] && sum == ent[LDIR_Chksum])
            {
                l = FatFileSystem::assembleLongName(l, ent);
                --ord;
            }
            else
            {
                ord = -1;       // Orphan
            }
        }
        else if (!FatFileSystem::isVolumeID(ent))
        {
            if (ord == 0 && sum == FatFileSystem::getChecksum(ent))
            {
                utf16cpy(fileName, l);  // FCB with a long-name
            }
            else
            {
                *fileName = 0;          // FCB without a long-name
            }
            return true;
        }
        else
        {
            ord = -1;
        }
    }

    if (freeSize < freeRequired)
    {
        if (freeOffset + freeSize != pos)
        {
            freeOffset = pos;
            freeSize = 0;
        }
        long long size;
        size = dir->getSize();
        if (size < freeOffset + freeRequired)
        {
            dir->setSize(freeOffset + freeRequired);    // XXX exception handling
        }
        freeSize = freeRequired;
    }
    return false;
}

bool FatStream::
findNext(es::Stream* dir, u8* ent, u16* fileName)
{
    int freeOffset;
    u32 freeSize;
    return findNext(dir, ent, fileName, 0, freeOffset, freeSize);
}

// The reference count of the looked up stream shall be incremented by one.
FatStream* FatStream::
lookup(FatStream* stream, const char*& name)
{
    stream->addRef();
    while (stream && name && *name != 0)
    {
        if (!stream->isDirectory())
        {
            stream->release();
            return 0;
        }

        const char* current = name;
        u16 fileName[256];
        name = FatFileSystem::splitPath(name, fileName);    // XXX missing length check

        if (fileName[0] == 0)
        {
            continue;
        }

        Handle<es::Stream> dir(stream->cache->getStream());
        bool found;
        FatStream* next = 0;
        {
            Synchronized<es::Monitor*> method(stream->monitor);

            u8 ent[32];
            u16 longName[256];
            while ((found = findNext(dir, ent, longName)))
            {
                if (FatFileSystem::isEqual(fileName, longName, ent))
                {
                    // Found fileName.
                    if (memcmp(ent + DIR_Name, FatFileSystem::nameDotdot, 11) == 0)
                    {
                        next = stream->parent;
                        next->addRef();
                    }
                    else if (memcmp(ent + DIR_Name, FatFileSystem::nameDot, 11) != 0)
                    {
                        long long pos;
                        pos = dir->getPosition();
                        next = stream->fileSystem->lookup(stream->fstClus, pos - 32);
                        if (!next)
                        {
                            next = new FatStream(stream->fileSystem, stream, pos - 32, ent);
                        }
                    }
                    break;
                }
            }
        }

        if (!found)
        {
            name = current;
            break;
        }

        if (next)
        {
            stream->release();
            stream = next;
        }
    }
    return stream;
}

FatStream* FatStream::
create(const char* name, u8 attr)
{
    Synchronized<es::Monitor*> method(monitor);

    u16 fileName[256];
    u16 longName[256];
    u16 oemName[256];
    u8 oem[32];
    u8 ent[32];
    bool lossy;
    int numericTrail;
    int numericBase;
    u32 numericMap;

    if (!isDirectory())
    {
        return 0;
    }

    if (isRemoved())
    {
        return 0;
    }

    FatFileSystem::utf8toutf16(name, fileName);    // XXX missing length check
    lossy = FatFileSystem::utf16tooem(fileName, oem);
    FatFileSystem::oemtoutf16(oem, oemName);

    // Calculate the required number of the directory entries. Note that
    // each entry can hold up to 13 Unicode characters.
    u32 freeRequired = 32;
    if (lossy)
    {
        freeRequired += 32 * ((utf16len(fileName) + 12) / 13);
    }
    int freeOffset = DIR_LIMIT;
    u32 freeSize = 0;
    int off = freeOffset;
    for (numericBase = 1; numericBase < 999999; numericBase += 32)
    {
        numericMap = 0;
        // Within the following loop, we need to determine
        // 1) there are sufficient number of free directory entries to hold
        // both the short name and the long name,
        // 2) the smallest free numeric-trail number for the short name, and
        // 3) the specified long name does not collide with the existing
        // short names and long names.
        Handle<es::Stream> dir(cache->getStream());
        while (findNext(dir, ent, longName, freeRequired, freeOffset, freeSize) &&
               numericMap != 0xffffffff)
        {
            if (0 < freeRequired && freeRequired <= freeSize)
            {
                freeRequired = 0;
                off = freeOffset;
            }

            if (FatFileSystem::isEqual(fileName, longName, ent))
            {
                // The specified long name collides with the existing entry name.
                return 0;
            }

            if (lossy)
            {
                numericTrail = FatFileSystem::getNumericTrail(oem, ent);
                if (0 < numericTrail)
                {
                    if (numericTrail - numericBase < 32)
                    {
                        numericMap |= 0x80000000u >> (numericTrail - numericBase);
                        if (numericMap == 0xffffffff)
                        {
                            break;
                        }
                    }
                }
            }
        }
        if (0 < freeRequired && freeRequired <= freeSize)
        {
            freeRequired = 0;
            off = freeOffset;
        }
        if (!lossy || numericMap != 0xffffffff)
        {
            break;
        }
    }
    if (DIR_LIMIT <= off)
    {
        // No free entries were found.
        return 0;
    }

    //
    // Create a new file
    //
    DateTime now = DateTime::getNow();

    // Prepare new FCB data
    memset(oem + DIR_CrtTimeTenth, 0, 32 - DIR_CrtTimeTenth);
    if (!(attr & ATTR_DIRECTORY))
    {
        xbyte(oem + DIR_Attr, ATTR_ARCHIVE);
    }
    else
    {
        xbyte(oem + DIR_Attr, ATTR_ARCHIVE | ATTR_DIRECTORY);
    }
    if (lossy)
    {
        // There ara name collisions.
        numericTrail = numericBase;
        if (numericMap)
        {
            numericTrail += 32 - ffs(numericMap);
        }
        FatFileSystem::setNumericTrail(oem, numericTrail);
    }

    u32 clus = 0;
    if (attr & ATTR_DIRECTORY)  // Create a diretory
    {
        // Allocate one cluster to the directory.
        clus = fileSystem->allocCluster(1, true);
    }
    xword(oem + DIR_FstClusLO, clus);
    xword(oem + DIR_FstClusHI, clus >> 16);

    freeRequired = 32;
    if (lossy)
    {
        freeRequired += 32 * ((utf16len(fileName) + 12) / 13);
    }
    FatStream* stream = new FatStream(fileSystem, this, off + freeRequired - 32, oem);
    {
        Synchronized<es::Monitor*> method(stream->monitor);  // this shold be safe.

        stream->setCreationTime(now);
        stream->setLastWriteTime(now);
        stream->setLastAccessTime(now);

        if (attr & ATTR_DIRECTORY)  // Create a diretory
        {
            Handle<es::Stream> dir(stream->cache->getStream());

            // dot
            memmove(ent + DIR_Name, FatFileSystem::nameDot, 11);
            xbyte(ent + DIR_Attr, ATTR_DIRECTORY);
            xword(ent + DIR_FstClusLO, stream->fstClus);
            xword(ent + DIR_FstClusHI, stream->fstClus >> 16);
            memmove(ent + DIR_CrtTimeTenth, stream->fcb + DIR_CrtTimeTenth, 5);
            memmove(ent + DIR_LstAccDate, stream->fcb + DIR_LstAccDate, 2);
            memmove(ent + DIR_WrtTime, stream->fcb + DIR_WrtTime, 4);
            dir->write(ent, 32);

            // dotdot
            memmove(ent + DIR_Name, FatFileSystem::nameDotdot, 11);
            xbyte(ent + DIR_Attr, ATTR_DIRECTORY);
            xword(ent + DIR_FstClusLO, fstClus);
            xword(ent + DIR_FstClusHI, fstClus >> 16);
            memmove(ent + DIR_CrtTimeTenth, stream->fcb + DIR_CrtTimeTenth, 5);
            memmove(ent + DIR_LstAccDate, stream->fcb + DIR_LstAccDate, 2);
            memmove(ent + DIR_WrtTime, stream->fcb + DIR_WrtTime, 4);
            dir->write(ent, 32);

            dir->flush();
        }
    }

    //
    // Fill in directory entries.
    //
    u8 sum = fileSystem->getChecksum(stream->fcb);
    Handle<es::Stream> dir(cache->getStream());
    dir->setPosition(off);
    if (lossy)
    {
        memset(ent, 0, 32);
        xbyte(ent + LDIR_Attr, ATTR_LONG_NAME);
        xbyte(ent + LDIR_Chksum, sum);
        int ord;
        for (ord = (freeRequired / 32 - 1) | LAST_LONG_ENTRY; 0 < ord; --ord)
        {
            xbyte(ent + LDIR_Ord, ord);
            FatFileSystem::fillLongName(ent, fileName, ord);
            if (dir->write(ent, 32) != 32)
            {
                // XXX Should clean up entires written so far.
                return 0;
            }
            ord &= ~LAST_LONG_ENTRY;
        }
    }
    if (dir->write(stream->fcb, 32) != 32)
    {
        // XXX Should clean up entires written so far.
        return 0;
    }

    dir->flush();    // XXX We must not update the diretory until here.

    return stream;
}

// remove() does not decrement the reference count of this stream.
int FatStream::
remove()
{
    Synchronized<es::Monitor*> method(monitor);

    const u8 e5 = 0xe5;

    if (isRemoved())
    {
        return 0;
    }

    if (isDirectory() && (isRoot() || !isEmpty()))
    {
        return -1;  // Not empty
    }

    fileSystem->remove(this);
    flags |= Removed;

    // Clear the directory entry including the long name entries
    Handle<es::Stream> dir(parent->cache->getStream());
    {
        Synchronized<es::Monitor*> method(parent->monitor);

        // XXX dir.setPosition(offset - XXX);
        u8 ent[32];
        while (dir->read(ent, 32) == 32 && ent[0] != 0x00)
        {
            if (FatFileSystem::isFreeEntry(ent))
            {
                continue;
            }

            long long pos;
            pos = dir->getPosition();

            if (FatFileSystem::isLongNameComponent(ent))
            {
                int ord = ent[LDIR_Ord] & ~LAST_LONG_ENTRY;
                if (pos + 32 * (ord - 1) == offset)
                {
                    dir->write(&e5, 1, pos - 32);
                }
            }
            else if (!FatFileSystem::isVolumeID(ent))
            {
                if (pos - 32 == offset)
                {
                    dir->write(&e5, 1, pos - 32);
                    break;
                }
            }
        }
        dir->flush();
    }

    return 0;
}
