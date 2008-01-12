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
#include <es/formatter.h>
#include "iso9660Stream.h"

bool Iso9660Stream::
canRead()
{
    return true;
}

bool Iso9660Stream::
canWrite()
{
    return false;
}

bool Iso9660Stream::
isDirectory()
{
    return (flags & FF_Directory) ? true : false;
}

bool Iso9660Stream::
isFile()
{
    return (flags & FF_Directory) ? false : true;
}

bool Iso9660Stream::
isHidden()
{
    return (flags & FF_Existence) ? true : false;
}

int Iso9660Stream::
getAttributes(unsigned int& attributes)
{
    u32 attr = IFile::ReadOnly;
    if (flags & FF_Existence)
    {
        attr |= IFile::Hidden;
    }
    if (flags & FF_Directory)
    {
        attr |= IFile::Directory;
    }
    attributes = attr;
    return 0;
}

int Iso9660Stream::
getCreationTime(long long& time)
{
    time = dateTime.getTicks();
    return 0;
}

int Iso9660Stream::
getLastAccessTime(long long& time)
{
    time = dateTime.getTicks();
    return 0;
}

int Iso9660Stream::
getLastWriteTime(long long& time)
{
    time = dateTime.getTicks();
    return 0;
}

int Iso9660Stream::
setAttributes(unsigned int attributes)
{
    return -1;
}

int Iso9660Stream::
setCreationTime(long long time)
{
    return -1;
}

int Iso9660Stream::
setLastAccessTime(long long time)
{
    return -1;
}

int Iso9660Stream::
setLastWriteTime(long long time)
{
    return -1;
}

IStream* Iso9660Stream::
getStream()
{
    if (isDirectory())
    {
        esThrow(EPERM);
    }
    return cache->getInputStream();
}

IPageable* Iso9660Stream::
getPageable()
{
    if (isDirectory())
    {
        esThrow(EPERM);
    }

    IPageable* pageable;
    cache->queryInterface(IID_IPageable, (void**) &pageable);
    return pageable;
}
