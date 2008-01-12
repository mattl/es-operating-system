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

DateTime FatStream::
getCreationTime()
{
    Synchronized<IMonitor*> method(monitor);

    try
    {
        u16 date = word(fcb + DIR_CrtDate);
        u16 time = word(fcb + DIR_CrtTime);
        u8 tenth = byte(fcb + DIR_CrtTimeTenth);
        return DateTime(1980 + (date >> 9), (date >> 5) & 0x0f, date & 0x1f,
                        time >> 11, (time >> 5) & 0x3f, (time & 0x1f) * 2 + tenth / 100,
                        (tenth % 100) * 10);
    }
    catch (...)
    {
        return FatFileSystem::epoch;
    }
}

DateTime FatStream::
getLastAccessTime()
{
    Synchronized<IMonitor*> method(monitor);

    try
    {
        u16 date = word(fcb + DIR_LstAccDate);
        return DateTime(1980 + (date >> 9), (date >> 5) & 0x0f, date & 0x1f);
    }
    catch (...)
    {
        return FatFileSystem::epoch;
    }
}

DateTime FatStream::
getLastWriteTime()
{
    Synchronized<IMonitor*> method(monitor);

    try
    {
        u16 date = word(fcb + DIR_WrtDate);
        u16 time = word(fcb + DIR_WrtTime);
        return DateTime(1980 + (date >> 9), (date >> 5) & 0x0f, date & 0x1f,
                        time >> 11, (time >> 5) & 0x3f, (time & 0x1f) * 2);
    }
    catch (...)
    {
        return FatFileSystem::epoch;
    }
}

void FatStream::
setCreationTime(DateTime d)
{
    Synchronized<IMonitor*> method(monitor);

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
    Synchronized<IMonitor*> method(monitor);

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
    Synchronized<IMonitor*> method(monitor);

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

int FatStream::
setAttributes(unsigned int attributes)
{
    Synchronized<IMonitor*> method(monitor);

    attributes &= (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_ARCHIVE);
    fcb[DIR_Attr] &= ~(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_ARCHIVE);
    fcb[DIR_Attr] |= attributes;
    return 0;
}

int FatStream::
setCreationTime(long long time)
{
    Synchronized<IMonitor*> method(monitor);

    try
    {
        DateTime d(time);
        setCreationTime(d);
    }
    catch (...)
    {
        return -1;
    }
    return 0;
}

int FatStream::
setLastAccessTime(long long time)
{
    Synchronized<IMonitor*> method(monitor);

    try
    {
        DateTime d(time);
        setLastAccessTime(d);
    }
    catch (...)
    {
        return -1;
    }
    return 0;
}

int FatStream::
setLastWriteTime(long long time)
{
    Synchronized<IMonitor*> method(monitor);

    try
    {
        DateTime d(time);
        setLastWriteTime(d);
    }
    catch (...)
    {
        return -1;
    }
    return 0;
}

int FatStream::
getAttributes(unsigned int& attributes)
{
    Synchronized<IMonitor*> method(monitor);

    attributes = fcb[DIR_Attr] & (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_DIRECTORY | ATTR_ARCHIVE);
    return 0;
}

int FatStream::
getCreationTime(long long& time)
{
    Synchronized<IMonitor*> method(monitor);

    DateTime d = getCreationTime();
    time = d.getTicks();
    return 0;
}

int FatStream::
getLastAccessTime(long long& time)
{
    Synchronized<IMonitor*> method(monitor);

    DateTime d = getLastAccessTime();
    time = d.getTicks();
    return 0;
}

int FatStream::
getLastWriteTime(long long& time)
{
    Synchronized<IMonitor*> method(monitor);

    DateTime d = getLastWriteTime();
    time = d.getTicks();
    return 0;
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

IStream* FatStream::
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

IPageable* FatStream::
getPageable()
{
    if (FatFileSystem::isDirectory(fcb))
    {
        esThrow(EPERM);
    }

    IPageable* pageable;
    cache->queryInterface(IID_IPageable, (void**) &pageable);
    return pageable;
}
