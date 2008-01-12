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

void* Stream::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == IStream::iid())
    {
        objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IFile::iid() && cache->file)
    {
        objectPtr = static_cast<IFile*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
}

unsigned int Stream::
addRef(void)
{
    return ref.addRef();
}

unsigned int Stream::
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
// IFile containment
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

int Stream::
getName(char* name, int nameLength)
{
    ASSERT(cache->file);
    return cache->file->getName(name, nameLength);
}

IPageable* Stream::
getPageable()
{
    ASSERT(cache->file);
    return cache->file->getPageable();
}

IStream* Stream::
getStream()
{
    ASSERT(cache->file);
    addRef();
    return this;
}
