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

//
// IContext
//

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include "fatStream.h"

namespace
{
    template <typename T>
    void swap(T& a, T& b)
    {
        T temp(a);
        a = b;
        b = temp;
    }
}

// object - In general, object is possibly null. For FatStream, however,
// object must be NULL.
es::Binding* FatStream::
bind(const char* name, Object* object)
{
    if (!canWrite())
    {
        esThrow(EACCES);
    }

    Handle<FatStream> stream(lookup(this, name));
    if (!stream || !name || *name == 0 || strchr(name, '/') || strchr(name, '\\'))
    {
        return 0;
    }
    return stream->create(name, 0);
}

es::Context* FatStream::
createSubcontext(const char* name)
{
    if (!canWrite())
    {
        esThrow(EACCES);
    }

    Handle<FatStream> stream(lookup(this, name));
    if (!stream || !name || *name == 0 || strchr(name, '/') || strchr(name, '\\'))
    {
        return 0;
    }
    return stream->create(name, ATTR_DIRECTORY);
}

int FatStream::
destroySubcontext(const char* name)
{
    if (!canWrite())
    {
        esThrow(EACCES);
    }

    Handle<FatStream> stream(lookup(this, name));
    if (!stream || !name || *name != 0)
    {
        return -1;
    }
    if (!stream->isDirectory())
    {
        return -1;
    }
    return stream->remove();    // XXX remove all entries under stream.
}

Object* FatStream::
lookup(const char* name)
{
    FatStream* stream(lookup(this, name));
    if (!name || *name != 0)
    {
        return 0;
    }
    return static_cast<es::Context*>(stream);
}

int FatStream::
rename(const char* oldName, const char* newName)
{
    if (!canWrite())
    {
        esThrow(EACCES);
    }

    Handle<FatStream> oldStream(lookup(this, oldName));
    if (!oldStream || oldStream->isRoot() || !oldName || *oldName != 0)
    {
        return -1;  // Not found
    }

    Handle<FatStream> newStream(lookup(this, newName));
    if (!newStream || !newName || *newName == 0 || strchr(newName, '/') || strchr(newName, '\\'))
    {
        return -1;
    }
    newStream = newStream->create(newName, oldStream->isDirectory() ? ATTR_DIRECTORY : 0);
    if (!newStream)
    {
        return -1;
    }

    // Swap newStream and oldStream data as we still need to use the oldStream pointer.
    fileSystem->remove(oldStream);
    fileSystem->remove(newStream);
    swap(oldStream->parent, newStream->parent);
    swap(oldStream->dirClus, newStream->dirClus);
    swap(oldStream->offset, newStream->offset);
    u8 tmp[32];
    memmove(tmp, oldStream->fcb, 13);
    memmove(oldStream->fcb, newStream->fcb, 13);
    memmove(newStream->fcb, tmp, 13);
    oldStream->flags |= Updated;
    newStream->flags |= Updated;
    fileSystem->add(oldStream);
    fileSystem->add(newStream);

    // Update '..'
    if (oldStream->parent != newStream->parent && oldStream->isDirectory())
    {
        Handle<es::Stream> dir(oldStream->cache->getStream());
        dir->read(tmp, 32, 32);
        xword(tmp + DIR_FstClusLO, oldStream->fstClus);
        xword(tmp + DIR_FstClusHI, oldStream->fstClus >> 16);
        dir->write(tmp, 32, 32);
        dir->flush();
    }

    oldStream->flush();

    return newStream->remove();
}

int FatStream::
unbind(const char* name)
{
    if (!canWrite())
    {
        esThrow(EACCES);
    }

    Handle<FatStream> stream(lookup(this, name));
    if (!stream || !name || *name != 0)
    {
        return -1;
    }
    if (stream->isDirectory())
    {
        return -1;
    }
    return stream->remove();
}

es::Iterator* FatStream::
list(const char* name)
{
    Handle<FatStream> stream(lookup(this, name));
    if (!stream || !name || *name != 0)
    {
        return 0;
    }
    return new FatIterator(stream);
}
