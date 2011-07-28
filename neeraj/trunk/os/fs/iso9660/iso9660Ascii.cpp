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

#include <string.h>
#include <es/handle.h>
#include "iso9660Stream.h"

// Get the first pathname component from path to file.
const char* Iso9660FileSystem::
splitPath(const char* path, char* file)
{
    while (path && *path)
    {
        if (isDelimitor(*path))
        {
            while (isDelimitor(*++path))
            {
            }
            break;
        }
        *file++ = *path++;
    }
    *file = 0;
    return path;
}

static void hideFileVersion(char* name, size_t len)
{
    while (0 < len--)
    {
        if (*name == ';')
        {
            // Ignore the file version.
            *name = 0;
        }
        ++name;
    }
}

// The reference count of the looked up stream shall be incremented by one.
Iso9660Stream* Iso9660Stream::
lookupPathName(const char*& name)
{
    Iso9660Stream* stream = this;
    stream->addRef();
    while (stream && name && *name != 0)
    {
        if (!stream->isDirectory())
        {
            stream->release();
            return 0;
        }

        const char* current = name;
        char fileName[256];
        name = Iso9660FileSystem::splitPath(name, fileName);    // XXX missing length check

        if (fileName[0] == 0 || strcmp(fileName, ".") == 0)
        {
            continue;
        }

        if (strcmp(fileName, "..") == 0)
        {
            if (!isRoot())
            {
                Iso9660Stream* next = stream->parent;
                next->addRef();
                stream->release();
                stream = next;
            }
            continue;
        }

        bool found;
        Handle<es::Stream> dir(stream->cache->getInputStream());
        u8 record[255];
        while (found = findNext(dir, record))
        {
            hideFileVersion((char*) record + DR_FileIdentifier, record[DR_FileIdentifierLength]);
            if (strncasecmp(fileName,
                            (char*) record + DR_FileIdentifier,
                            record[DR_FileIdentifierLength]) == 0)
            {
                // Found fileName.
                long long pos;
                pos = dir->getPosition();
                Iso9660Stream* next = fileSystem->lookup(location, pos - record[DR_Length]);
                if (!next)
                {
                    next = fileSystem->createStream(fileSystem, stream, pos - record[DR_Length], record);
                }
                stream->release();
                stream = next;
                break;
            }
        }
        if (!found)
        {
            name = current;
            break;
        }
    }
    return stream;
}

const char* Iso9660Stream::
getName(void* name, int len)
{
    u8 record[255];

    if (len == 0)
    {
        return 0;
    }
    if (isRoot())
    {
        static_cast<char*>(name)[0] = 0;
    }
    else
    {
        Handle<es::Stream> dir(parent->cache->getInputStream());
        dir->setPosition(offset);
        if (!parent->findNext(dir, record))
        {
            return 0;
        }
        if (record[DR_FileIdentifierLength] < len)
        {
            memmove(name, record + DR_FileIdentifier, record[DR_FileIdentifierLength]);
            static_cast<char*>(name)[record[DR_FileIdentifierLength]] = 0;
        }
        else
        {
            memmove(name, record + DR_FileIdentifier, len);
        }
    }
    return static_cast<char*>(name);
}
