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

// ISO 9660 directory iterator

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include "iso9660Stream.h"

Iso9660Iterator::
Iso9660Iterator(Iso9660Stream* stream) :
    ref(1), stream(stream)
{
    ASSERT(stream->isDirectory());
    stream->addRef();
    ipos = 0;
}

Iso9660Iterator::
~Iso9660Iterator()
{
    stream->release();
}

bool Iso9660Iterator::
hasNext(void)
{
    ASSERT(stream->isDirectory());
    Handle<IStream> dir(stream->cache->getInputStream());
    dir->setPosition(ipos);

    u8 record[255];
    return stream->findNext(dir, record);
}

// Dot and dotdot entries are not reported.
IInterface* Iso9660Iterator::
next()
{
    ASSERT(stream->isDirectory());
    Handle<IStream> dir(stream->cache->getInputStream());
    dir->setPosition(ipos);

    u8 record[255];
    if (!stream->findNext(dir, record))
    {
        return 0;
    }

    ipos = dir->getPosition();
    Iso9660Stream* next = stream->fileSystem->lookup(stream->location, ipos - record[DR_Length]);
    if (!next)
    {
        next = stream->fileSystem->createStream(stream->fileSystem, stream, ipos - record[DR_Length], record);
    }
    return static_cast<IBinding*>(next);
}

int Iso9660Iterator::
remove(void)
{
    return -1;
}

bool Iso9660Iterator::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IIterator)
    {
        *objectPtr = static_cast<IIterator*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IIterator*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Iso9660Iterator::
addRef(void)
{
    return ref.addRef();
}

unsigned int Iso9660Iterator::
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
