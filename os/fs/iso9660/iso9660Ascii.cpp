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

#include <string.h>
#include <es/handle.h>
#include "iso9660Stream.h"

extern "C" int strnicmp(const char *s1, const char *s2, size_t n);

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
        Handle<IStream> dir(stream->cache->getInputStream());
        u8 record[255];
        while (found = findNext(dir, record))
        {
            hideFileVersion((char*) record + DR_FileIdentifier, record[DR_FileIdentifierLength]);
            if (strnicmp(fileName,
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

int Iso9660Stream::
getName(char* name, int len)
{
    u8 record[255];

    if (len == 0)
    {
        return 0;
    }
    if (isRoot())
    {
        name[0] = 0;
    }
    else
    {
        Handle<IStream> dir(parent->cache->getInputStream());
        dir->setPosition(offset);
        if (!parent->findNext(dir, record))
        {
            return -1;
        }
        if (record[DR_FileIdentifierLength] < len)
        {
            memmove(name, record + DR_FileIdentifier, record[DR_FileIdentifierLength]);
            name[record[DR_FileIdentifierLength]] = 0;
        }
        else
        {
            memmove(name, record + DR_FileIdentifier, len);
        }
    }
    return 0;
}
