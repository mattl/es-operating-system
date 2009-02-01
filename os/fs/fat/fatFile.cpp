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

// IFile

#include <errno.h>
#include <string.h>
#include <es.h>
#include "fatStream.h"

const DateTime FatFileSystem::epoch(1980, 1, 1);
const DateTime FatFileSystem::wane(2107, 12, 31, 23, 59, 59, 99);

void FatStream::
setCreationTime(DateTime d)
{
    Synchronized<es::Monitor*> method(monitor);

    if (d < FatFileSystem::epoch)
    {
        d = FatFileSystem::epoch;
    }
    else if (FatFileSystem::wane < d)
    {
        d = FatFileSystem::wane;
    }
    u16 date = ((d.getYear() - 1980) << 9) | (d.getMonth() << 5) | d.getDay();
    u16 time = (d.getHour() << 11) | (d.getMinute() << 5) | (d.getSecond() / 2);
    u16 tenth = d.getSecond() % 2 * 100 + d.getMillisecond() / 10;
    xword(fcb + DIR_CrtDate, date);
    xword(fcb + DIR_CrtTime, time);
    xbyte(fcb + DIR_CrtTimeTenth, tenth);
    flags |= Updated;
}

void FatStream::
setLastAccessTime(DateTime d)
{
    Synchronized<es::Monitor*> method(monitor);

    if (d < FatFileSystem::epoch)
    {
        d = FatFileSystem::epoch;
    }
    else if (FatFileSystem::wane < d)
    {
        d = FatFileSystem::wane;
    }
    u16 date = ((d.getYear() - 1980) << 9) | (d.getMonth() << 5) | d.getDay();
    xword(fcb + DIR_LstAccDate, date);
    flags |= Updated;
}

void FatStream::
setLastWriteTime(DateTime d)
{
    Synchronized<es::Monitor*> method(monitor);

    if (d < FatFileSystem::epoch)
    {
        d = FatFileSystem::epoch;
    }
    else if (FatFileSystem::wane < d)
    {
        d = FatFileSystem::wane;
    }
    u16 date = ((d.getYear() - 1980) << 9) | (d.getMonth() << 5) | d.getDay();
    u16 time = (d.getHour() << 11) | (d.getMinute() << 5) | (d.getSecond() / 2);
    xword(fcb + DIR_WrtDate, date);
    xword(fcb + DIR_WrtTime, time);
    flags |= Updated;
}

void FatStream::
setAttributes(unsigned int attributes)
{
    Synchronized<es::Monitor*> method(monitor);

    attributes &= (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_ARCHIVE);
    fcb[DIR_Attr] &= ~(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_ARCHIVE);
    fcb[DIR_Attr] |= attributes;
}

void FatStream::
setCreationTime(long long time)
{
    Synchronized<es::Monitor*> method(monitor);

    try
    {
        DateTime d(time);
        setCreationTime(d);
    }
    catch (...)
    {
        // [check] throw exception.
    }
}

void FatStream::
setLastAccessTime(long long time)
{
    Synchronized<es::Monitor*> method(monitor);

    try
    {
        DateTime d(time);
        setLastAccessTime(d);
    }
    catch (...)
    {
        // [check] throw exception.
    }
}

void FatStream::
setLastWriteTime(long long time)
{
    Synchronized<es::Monitor*> method(monitor);

    try
    {
        DateTime d(time);
        setLastWriteTime(d);
    }
    catch (...)
    {
        // [check] throw exception.
    }
}

unsigned int FatStream::
getAttributes()
{
    Synchronized<es::Monitor*> method(monitor);

    return fcb[DIR_Attr] & (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_DIRECTORY | ATTR_ARCHIVE);
}

long long FatStream::
getCreationTime()
{
    Synchronized<es::Monitor*> method(monitor);

    DateTime d;
    try
    {
        u16 date = word(fcb + DIR_CrtDate);
        u16 time = word(fcb + DIR_CrtTime);
        u8 tenth = byte(fcb + DIR_CrtTimeTenth);
        d = DateTime(1980 + (date >> 9), (date >> 5) & 0x0f, date & 0x1f,
                        time >> 11, (time >> 5) & 0x3f, (time & 0x1f) * 2 + tenth / 100,
                        (tenth % 100) * 10);
    }
    catch (...)
    {
        d = FatFileSystem::epoch;
    }

    return d.getTicks();
}

long long FatStream::
getLastAccessTime()
{
    Synchronized<es::Monitor*> method(monitor);

    DateTime d;
    try
    {
        u16 date = word(fcb + DIR_LstAccDate);
        d = DateTime(1980 + (date >> 9), (date >> 5) & 0x0f, date & 0x1f);
    }
    catch (...)
    {
        d = FatFileSystem::epoch;
    }

    return d.getTicks();
}

long long FatStream::
getLastWriteTime()
{
    Synchronized<es::Monitor*> method(monitor);

    DateTime d;
    try
    {
        u16 date = word(fcb + DIR_WrtDate);
        u16 time = word(fcb + DIR_WrtTime);
        d = DateTime(1980 + (date >> 9), (date >> 5) & 0x0f, date & 0x1f,
                        time >> 11, (time >> 5) & 0x3f, (time & 0x1f) * 2);
    }
    catch (...)
    {
        d = FatFileSystem::epoch;
    }

    return d.getTicks();
}

bool FatStream::
canRead()
{
    return FatFileSystem::canRead(fcb);
}

bool FatStream::
canWrite()
{
    return FatFileSystem::canWrite(fcb);
}

bool FatStream::
isDirectory()
{
    return FatFileSystem::isDirectory(fcb);
}

bool FatStream::
isFile()
{
    return FatFileSystem::isFile(fcb);
}

bool FatStream::
isHidden()
{
    return FatFileSystem::isHidden(fcb);
}

es::Stream* FatStream::
getStream()
{
    if (FatFileSystem::isDirectory(fcb))
    {
        esThrow(EPERM);
    }
    if (canWrite())
    {
        return cache->getStream();
    }
    else
    {
        return cache->getInputStream();
    }
}

es::Pageable* FatStream::
getPageable()
{
    if (FatFileSystem::isDirectory(fcb))
    {
        esThrow(EPERM);
    }

    es::Pageable* pageable;
    pageable = reinterpret_cast<es::Pageable*>(cache->queryInterface(es::Pageable::iid()));
    return pageable;
}
