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

#include <new>
#include <errno.h>
#include "cache.h"

Stream::
Stream(Cache* cache) :
    cache(cache), position(0)
{
    cache->addRef();
}

Stream::
~Stream()
{
    cache->release();
}

long long Stream::
getPosition()
{
    Monitor::Synchronized method(monitor);
    return position;
}

void Stream::
setPosition(long long pos)
{
    Monitor::Synchronized method(monitor);
    long long size;

    size = getSize();
    if (size < pos)
    {
        pos = size;
    }
    position = pos;
}

long long Stream::
getSize()
{
    return cache->getSize();
}

void Stream::
setSize(long long size)
{
    cache->setSize(size);
}

int Stream::
read(void* dst, int count)
{
    Monitor::Synchronized method(monitor);
    int n = cache->read(dst, count, position);
    if (0 < n)
    {
        setPosition(position + n);
    }
    return n;
}

int Stream::
read(void* dst, int count, long long offset)
{
    return cache->read(dst, count, offset);
}

int Stream::
write(const void* src, int count)
{
    Monitor::Synchronized method(monitor);
    int n = cache->write(src, count, position);
    if (0 < n)
    {
        setPosition(position + n);
    }
    return n;
}

int Stream::
write(const void* src, int count, long long offset)
{
    return cache->write(src, count, offset);
}

void Stream::
flush()
{
    cache->flush();
}

Object* Stream::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Stream::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else if (strcmp(riid, es::File::iid()) == 0 && cache->file)
    {
        objectPtr = static_cast<es::File*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Stream::
addRef()
{
    return ref.addRef();
}

unsigned int Stream::
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

InputStream::
InputStream(Cache* cache) :
    Stream(cache)
{
}

InputStream::
~InputStream()
{
}

void InputStream::
setSize(long long size)
{
    esThrow(EACCES);
}

int InputStream::
write(const void* src, int count)
{
    esThrow(EACCES);
}

int InputStream::
write(const void* src, int count, long long offset)
{
    esThrow(EACCES);
}

void InputStream::
flush()
{
    esThrow(EACCES);
}

OutputStream::
OutputStream(Cache* cache) :
    Stream(cache)
{
}

OutputStream::
~OutputStream()
{
}

int OutputStream::
read(void* dst, int count)
{
    esThrow(EACCES);
}

int OutputStream::
read(void* dst, int count, long long offset)
{
    esThrow(EACCES);
}

//
// es::File containment
//

unsigned int Stream::
getAttributes()
{
    ASSERT(cache->file);
    return cache->file->getAttributes();
}

void Stream::
setAttributes(unsigned int attributes)
{
    ASSERT(cache->file);
    return cache->file->setAttributes(attributes);
}

long long Stream::
getCreationTime()
{
    ASSERT(cache->file);
    return cache->file->getCreationTime();
}

void Stream::
setCreationTime(long long creationTime)
{
    ASSERT(cache->file);
    return cache->file->setCreationTime(creationTime);
}

long long Stream::
getLastAccessTime()
{
    ASSERT(cache->file);
    return cache->file->getLastAccessTime();
}

void Stream::
setLastAccessTime(long long lastAccessTime)
{
    ASSERT(cache->file);
    return cache->file->setLastAccessTime(lastAccessTime);
}

long long Stream::
getLastWriteTime()
{
    ASSERT(cache->file);
    return cache->file->getLastWriteTime();
}

void Stream::
setLastWriteTime(long long lastWriteTime)
{
    ASSERT(cache->file);
    return cache->file->setLastWriteTime(lastWriteTime);
}

bool Stream::
canRead()
{
    ASSERT(cache->file);
    return cache->file->canRead();
}

bool Stream::
canWrite()
{
    ASSERT(cache->file);
    return cache->file->canWrite();
}

bool Stream::
isDirectory()
{
    ASSERT(cache->file);
    return cache->file->isDirectory();
}

bool Stream::
isFile()
{
    ASSERT(cache->file);
    return cache->file->isFile();
}

bool Stream::
isHidden()
{
    ASSERT(cache->file);
    return cache->file->isHidden();
}

const char* Stream::
getName(void* name, int nameLength)
{
    ASSERT(cache->file);
    return cache->file->getName(name, nameLength);
}

es::Pageable* Stream::
getPageable()
{
    ASSERT(cache->file);
    return cache->file->getPageable();
}

es::Stream* Stream::
getStream()
{
    ASSERT(cache->file);
    addRef();
    return this;
}
