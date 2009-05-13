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

// IStream

#include <errno.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include "fatStream.h"

u32 FatStream::
getClusNum(long long position)
{
    ASSERT(0 <= position);
    if (fstClus == 0)
    {
        // This stream represents the root directory of FAT12/FAT16 file system.
        ASSERT(!fileSystem->isFat32());
        return fstClus;
    }

    ASSERT(position < 0x100000000LL);
    position &= ~(fileSystem->bytsPerClus - 1);

    u32 clus;
    u32 i;
    if (position < lastPosition || lastClus == 0 || fileSystem->isEof(lastClus))
    {
        clus = fstClus;
        i = (u32) (position / fileSystem->bytsPerClus);
    }
    else
    {
        clus = lastClus;
        i = (u32) ((position - lastPosition) / fileSystem->bytsPerClus);
    }

    while (0 < i)
    {
        clus = fileSystem->clusEntryVal(clus);
        if (fileSystem->isEof(clus))
        {
            break;
        }
        --i;
    }

    lastPosition = position;
    lastClus = clus;

    return clus;
}

long long FatStream::
getPosition()
{
    return 0;
}

void FatStream::
setPosition(long long pos)
{
}

long long FatStream::
getSize()
{
    Synchronized<es::Monitor*> method(monitor);

    return this->size;
}

void FatStream::
setSize(long long newSize)
{
    Synchronized<es::Monitor*> method(monitor);

    if (newSize < 0 || 0xffffffff < newSize)
    {
        esThrow(EINVAL);
    }

    if (isDirectory())
    {
        newSize = (newSize + fileSystem->bytsPerClus - 1) & ~(fileSystem->bytsPerClus - 1);
        if (DIR_LIMIT < newSize || (!fileSystem->isFat32() && isRoot()))
        {
            esThrow(EINVAL);
        }
    }

    if (size == newSize)
    {
        return;
    }

    if (size < newSize)
    {
        u32 n;
        n = (newSize + fileSystem->bytsPerClus - 1) / fileSystem->bytsPerClus;
        n -= (size + fileSystem->bytsPerClus - 1) / fileSystem->bytsPerClus;
        if (0 < n)
        {
            // If this stream is a directory, we should zero-fill the content of
            // the new cluster before it is linked to the cluster chain.
            u32 clus = fileSystem->allocCluster(n, isDirectory() ? true : false);
            if (fileSystem->isEof(clus))
            {
                esThrow(ENOSPC);
            }
            if (size)
            {
                u32 lastClus = getClusNum(size - 1);
                ASSERT(!fileSystem->isEof(lastClus));
                fileSystem->setClusEntryVal(lastClus, clus);
            }
            else
            {
                ASSERT(fstClus == 0);
                fstClus = clus;
                xword(fcb + DIR_FstClusLO, fstClus);
                xword(fcb + DIR_FstClusHI, fstClus >> 16);
            }
        }

        xdword(fcb + DIR_FileSize, newSize);
        DateTime now = DateTime::getNow();
        setLastWriteTime(now);
        setLastAccessTime(now);
        flush();
    }
    else if (newSize < size)
    {
        xdword(fcb + DIR_FileSize, newSize);
        DateTime now = DateTime::getNow();
        setLastWriteTime(now);
        setLastAccessTime(now);

        u32 clus;
        if (newSize == 0)
        {
            clus = fstClus;
            fstClus = 0;
            xword(fcb + DIR_FstClusLO, fstClus);
            xword(fcb + DIR_FstClusHI, fstClus);
            flush();
            fileSystem->freeCluster(clus);
        }
        else
        {
            flush();
            clus = getClusNum(newSize - 1);
            u32 next = fileSystem->clusEntryVal(clus);
            fileSystem->setClusEntryVal(clus, 0x0fffffff);
            fileSystem->freeCluster(next);
        }

        // Reset getClusNum() acceleration anyways.
        lastPosition = 0;
        lastClus = fstClus;
    }

    size = newSize;
}

int FatStream::
read(void* dst, int count)
{
    return -1;
}

int FatStream::
read(void* dst, int count, long long offset)
{
    Synchronized<es::Monitor*> method(monitor);

    if (size < offset || count < 0)
    {
        esThrow(EINVAL);
    }
    if (size - offset < count)
    {
        count = size - offset;
    }
    if (count == 0)
    {
        return 0;
    }

    int len;
    int n;
    for (len = 0; len < count; len += n, offset += n)
    {
        u32 clus = getClusNum(offset);
        n = fileSystem->readCluster((u8*) dst + len,
                                    count - len,
                                    clus,
                                    clus ? (offset % fileSystem->bytsPerClus) : offset);
        if (n <= 0)
        {
            break;
        }
    }
    if (count < len)
    {
        len = count;
    }

    if (0 < len && !isDirectory())
    {
        DateTime now = DateTime::getNow();
        setLastAccessTime(now);
    }
    return len;
}

int FatStream::
write(const void* src, int count)
{
    return -1;
}

int FatStream::
write(const void* src, int count, long long offset)
{
    Synchronized<es::Monitor*> method(monitor);

    if (size < offset || count < 0)
    {
        esThrow(EINVAL);
    }
    if (size - offset < count)
    {
        count = size - offset;
    }
    if (count == 0)
    {
        return 0;
    }

    int len;
    int n;
    for (len = 0; len < count; len += n, offset += n)
    {
        u32 clus = getClusNum(offset);
        n = fileSystem->writeCluster((u8*) src + len,
                                     count - len,
                                     clus,
                                     clus ? (offset % fileSystem->bytsPerClus) : offset);
        if (n <= 0)
        {
            break;
        }
    }
    if (count < len)
    {
        len = count;
    }

    if (0 < len && !isDirectory())
    {
        DateTime now = DateTime::getNow();
        setLastWriteTime(now);
        setLastAccessTime(now);
    }
    return len;
}

void FatStream::
flush()
{
    Synchronized<es::Monitor*> method(monitor);

    if (flags & Updated)
    {
        flags &= ~Updated;
        if (!isRemoved())
        {
            fcb[DIR_Attr] |= ATTR_ARCHIVE;
            if (parent)
            {
                Handle<es::Stream> dir(parent->cache->getStream());
                dir->write(fcb, 32, offset);
                dir->flush();
            }
        }
    }
}

FatStream::
FatStream(FatFileSystem* fileSystem, FatStream* parent, u32 offset, u8* fcb) :
    ref(2),     // Plus one for fileSystem->standbyList management
    fileSystem(fileSystem),
    cache(0),
    parent(parent),
    offset(offset),
    flags(0)
{
    ASSERT(memcmp(fcb, FatFileSystem::nameDot, 11) != 0);
    ASSERT(memcmp(fcb, FatFileSystem::nameDotdot, 11) != 0);

    monitor = es::Monitor::createInstance();

    memmove(this->fcb, fcb, 32);
    fstClus = word(fcb + DIR_FstClusLO) | (word(fcb + DIR_FstClusHI) << 16);
    if (!isDirectory())
    {
        size = dword(fcb + DIR_FileSize);
    }
    else
    {
        size = fileSystem->calcSize(fstClus);
    }
    if (parent)
    {
        parent->addRef();
        dirClus = parent->fstClus;
    }
    else
    {
        dirClus = 0;
    }

    if (!isDirectory())
    {
        cache = es::Cache::createInstance(this);
    }
    else
    {
        cache = es::Cache::createInstance(this, fileSystem->pageSet);
    }
    cache->setSectorSize(fileSystem->bytsPerClus);
    fileSystem->add(this);

    lastPosition = 0;
    lastClus = fstClus;

    ref.release();  // Revert to normal
}

FatStream::
~FatStream()
{
    ASSERT(ref == 0);

    if (cache)
    {
        if (isRemoved())
        {
            cache->invalidate();
        }
        else
        {
            cache->flush();
        }
        cache->release();
    }

    if (parent)
    {
        parent->release();
    }

    monitor->release();
}

Object* FatStream::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (isDirectory() && strcmp(riid, es::Context::iid()) == 0)
    {
        objectPtr = static_cast<es::Context*>(this);
    }
    else if (strcmp(riid, es::File::iid()) == 0)
    {
        objectPtr = static_cast<es::File*>(this);
    }
    else if (strcmp(riid, es::Binding::iid()) == 0)
    {
        objectPtr = static_cast<es::Binding*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Binding*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int FatStream::
addRef()
{
    unsigned int count = ref.addRef();
    if (count == 2)
    {
        // This stream has been standing by.
        fileSystem->activate(this);
    }
    return count;
}

unsigned int FatStream::
release()
{
    unsigned int count;

    if (isRemoved())
    {
        count = ref.release();
        if (count == 0)
        {
            delete this;
        }
        else if (count == 1)
        {
            if (cache)
            {
                es::Cache* c = cache;

                cache = 0;
                c->invalidate();
                c->release();
            }
            fileSystem->freeCluster(fstClus);
            if (parent)
            {
                parent->release();
                parent = 0;
            }
        }
    }
    else
    {
        cache->flush();
        count = ref.release();
        if (count == 1)
        {
            fileSystem->standBy(this);
        }
    }
    return count;
}
