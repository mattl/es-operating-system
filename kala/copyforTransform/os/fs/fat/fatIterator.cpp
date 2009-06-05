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
    Synchronized<es::Monitor*> method(stream->monitor);

    ASSERT(stream->isDirectory());
    Handle<es::Stream> dir(stream->cache->getStream());
    dir->setPosition(ipos);

    u8 fcb[32];
    u16 longName[256];
    return stream->findNext(dir, fcb, longName);
}

// Dot and dotdot entries are not reported.
Object* FatIterator::
next()
{
    Synchronized<es::Monitor*> method(stream->monitor);

    ASSERT(stream->isDirectory());
    Handle<es::Stream> dir(stream->cache->getStream());
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
    return static_cast<es::Binding*>(next);
}

// XXX wrong semantics?
int FatIterator::
remove(void)
{
    Synchronized<es::Monitor*> method(stream->monitor);

    Object* found = next();
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

Object* FatIterator::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Iterator::iid()) == 0)
    {
        objectPtr = static_cast<es::Iterator*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Iterator*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int FatIterator::
addRef()
{
    return ref.addRef();
}

unsigned int FatIterator::
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
