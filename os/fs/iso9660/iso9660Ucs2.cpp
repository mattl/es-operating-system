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
#include <es.h>
#include <es/handle.h>
#include "iso9660Stream.h"

// #define VERBOSE

const u16 Iso9660StreamUcs2::dot[2] =
{
    '.', 0
};

const u16 Iso9660StreamUcs2::dotdot[3] =
{
    '.', '.', 0
};

static void utf16betoh(u16* utf16be, size_t len)
{
    while (0 < len--)
    {
        *utf16be = BigEndian::word((u8*) utf16be);
        if (*utf16be == L';')
        {
            // Ignore the file version.
            *utf16be = 0;
        }
        ++utf16be;
    }
}

// Get the first pathname component from utf8 to utf16.
const char* Iso9660FileSystem::
splitPath(const char* utf8, u16* utf16)
{
    while (utf8 && *utf8)
    {
        if (isDelimitor(*utf8))
        {
            while (isDelimitor(*++utf8))
            {
            }
            break;
        }
        u32 utf32;
        utf8 = utf8to32(utf8, &utf32);
        utf16 = utf32to16(utf32, utf16);
    }
    *utf16 = 0;
    return utf8;
}

// The reference count of the looked up stream shall be incremented by one.
Iso9660Stream* Iso9660StreamUcs2::
lookupPathName(const char*& name)
{
#ifdef VERBOSE
    esReport("Iso9660StreamUcs2::%s(%s)\n", __func__, name);
#endif

    Iso9660Stream* stream = this;
    stream->addRef();
    while (stream && name && *name != 0)
    {
#ifdef VERBOSE
        esReport("    Iso9660StreamUcs2::%s - %s\n", __func__, name);
#endif
        if (!stream->isDirectory())
        {
            stream->release();
            return 0;
        }

        const char* current = name;
        u16 fileName[256];
        name = Iso9660FileSystem::splitPath(name, fileName);    // XXX missing length check

        if (fileName[0] == 0 || utf16cmp(fileName, dot) == 0)
        {
            continue;
        }

        if (utf16cmp(fileName, dotdot) == 0)
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

        size_t fileNameLen = utf16len(fileName);

        bool found;
        Handle<es::Stream> dir(stream->cache->getInputStream());
        u8 record[255];
        while (found = findNext(dir, record))
        {
            ASSERT(record[DR_FileIdentifierLength] % 2 == 0);
            utf16betoh((u16*) (record + DR_FileIdentifier),
                       record[DR_FileIdentifierLength] / 2);

#ifdef VERBOSE
            esReport("(%d)\n", record[DR_FileIdentifierLength]);
            esDump(record, record[DR_Length]);
#endif

            if (fileNameLen <= record[DR_FileIdentifierLength] / 2 &&
                utf16nicmp(fileName, (u16*) (record + DR_FileIdentifier), fileNameLen) == 0 &&
                (fileNameLen == record[DR_FileIdentifierLength] / 2 ||
                 ((u16*) (record + DR_FileIdentifier))[fileNameLen] == 0x00 ||  // undocumented?
                 ((u16*) (record + DR_FileIdentifier))[fileNameLen] == 0x3b))   // separator 2
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

const char* Iso9660StreamUcs2::
getName(char* name, int len)
{
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
        u8 record[255];

        Handle<es::Stream> dir(parent->cache->getInputStream());
        dir->setPosition(offset);
        if (!parent->findNext(dir, record))
        {
            return 0;
        }

        char* dst = name;
        u16* src = (u16*) (record + DR_FileIdentifier);
        ASSERT(record[DR_FileIdentifierLength] % 2 == 0);
        u8 ilen = record[DR_FileIdentifierLength] / 2;
        utf16betoh(src, ilen);
        while (0 < len && 0 < ilen && src && *src != 0)
        {
            u32 utf32;
            src = utf16to32(src, &utf32);
            size_t n = utf32to8len(utf32);
            if (len < n)
            {
                break;
            }
            len -= n;
            dst = utf32to8(utf32, dst);
            --ilen;
        }
        if (0 < len)
        {
            *dst++ = 0;
        }
    }
    return name;
}
