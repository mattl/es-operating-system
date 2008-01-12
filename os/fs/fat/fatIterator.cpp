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

#include <ctype.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include "fatStream.h"

FatIterator::
FatIterator(FatStream* stream) :
    stream(stream)
{
    ASSERT(stream->isDirectory());
    stream->addRef();
    if (stream->isRoot())
    {
        ipos = 0;
    }
    else
    {
        ipos = 2 * 32;      // Skip dot and dotdot.
    }
}

FatIterator::
~FatIterator()
{
    stream->release();
}

bool FatIterator::
hasNext(void)
{
    Synchronized<IMonitor*> method(stream->monitor);

    ASSERT(stream->isDirectory());
    Handle<IStream> dir(stream->cache->getStream());
    dir->setPosition(ipos);

    u8 fcb[32];
    u16 longName[256];
    return stream->findNext(dir, fcb, longName);
}

// Dot and dotdot entries are not reported.
IInterface* FatIterator::
next()
{
    Synchronized<IMonitor*> method(stream->monitor);

    ASSERT(stream->isDirectory());
    Handle<IStream> dir(stream->cache->getStream());
    dir->setPosition(ipos);

    u8 fcb[32];
    u16 longName[256];
    if (!stream->findNext(dir, fcb, longName))
    {
        return 0;
    }

    ipos = dir->getPosition();
    FatStream* next = stream->fileSystem->lookup(stream->fstClus, ipos - 32);
    if (!next)
    {
        next = new FatStream(stream->fileSystem, stream, ipos - 32, fcb);
    }
    return static_cast<IBinding*>(next);
}

// XXX wrong semantics?
int FatIterator::
remove(void)
{
    Synchronized<IMonitor*> method(stream->monitor);

    IInterface* found = next();
    if (found)
    {
        FatStream* s = dynamic_cast<FatStream*>(found);
        if (s)
        {
            s->remove();
        }
        found->release();
    }
    return 0;
}

bool FatIterator::
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

unsigned int FatIterator::
addRef(void)
{
    return ref.addRef();
}

unsigned int FatIterator::
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
