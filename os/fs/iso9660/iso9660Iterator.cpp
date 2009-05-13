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
hasNext()
{
    ASSERT(stream->isDirectory());
    Handle<es::Stream> dir(stream->cache->getInputStream());
    dir->setPosition(ipos);

    u8 record[255];
    return stream->findNext(dir, record);
}

// Dot and dotdot entries are not reported.
Object* Iso9660Iterator::
next()
{
    ASSERT(stream->isDirectory());
    Handle<es::Stream> dir(stream->cache->getInputStream());
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
    return static_cast<es::Binding*>(next);
}

int Iso9660Iterator::
remove()
{
    return -1;
}

Object* Iso9660Iterator::
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

unsigned int Iso9660Iterator::
addRef()
{
    return ref.addRef();
}

unsigned int Iso9660Iterator::
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
