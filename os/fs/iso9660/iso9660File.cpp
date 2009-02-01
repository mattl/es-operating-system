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

unsigned int Iso9660Stream::
getAttributes()
{
    u32 attr = es::File::ReadOnly;
    if (flags & FF_Existence)
    {
        attr |= es::File::Hidden;
    }
    if (flags & FF_Directory)
    {
        attr |= es::File::Directory;
    }

    return attr;
}

long long Iso9660Stream::
getCreationTime()
{
    return dateTime.getTicks();
}

long long Iso9660Stream::
getLastAccessTime()
{
    return dateTime.getTicks();
}

long long Iso9660Stream::
getLastWriteTime()
{
    return dateTime.getTicks();
}

void Iso9660Stream::
setAttributes(unsigned int attributes)
{
    esThrow(EROFS);
}

void Iso9660Stream::
setCreationTime(long long time)
{
    esThrow(EROFS);
}

void Iso9660Stream::
setLastAccessTime(long long time)
{
    esThrow(EROFS);
}

void Iso9660Stream::
setLastWriteTime(long long time)
{
    esThrow(EROFS);
}

es::Stream* Iso9660Stream::
getStream()
{
    if (isDirectory())
    {
        esThrow(EPERM);
    }
    return cache->getInputStream();
}

es::Pageable* Iso9660Stream::
getPageable()
{
    if (isDirectory())
    {
        esThrow(EPERM);
    }

    es::Pageable* pageable;
    pageable = reinterpret_cast<es::Pageable*>(cache->queryInterface(es::Pageable::iid()));
    return pageable;
}
