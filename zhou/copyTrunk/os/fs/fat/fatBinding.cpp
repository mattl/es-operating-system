/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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
// IBinding
//

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include "fatStream.h"

Object* FatStream::
getObject()
{
    addRef();
    return static_cast<es::Context*>(this);
}

void FatStream::
setObject(Object* object)
{
    esThrow(EACCES); // [check]
}

const char* FatStream::
getName(void* name, int len)
{
    Synchronized<es::Monitor*> method(monitor);

    int ord = -1;   // The order of long-name entry
    u8 sum;
    u16 longName[13 * 20 + 1];
    u16* l;

    if (1 <= len)
    {
        static_cast<char*>(name)[0] = 0;
    }

    if (isRoot() || isRemoved())
    {
        return 0;
    }

    // Clear the directory entry including the long name entries
    Handle<es::Stream> dir(parent->cache->getStream());
    {
        Synchronized<es::Monitor*> method(parent->monitor);

        u8 fcb[32];
        while (dir->read(fcb, 32) == 32 && fcb[0] != 0x00)
        {
            if (ord < 0)
            {
                l = &longName[13 * 20 + 1];
                *--l = 0;
            }
            if (FatFileSystem::isFreeEntry(fcb))
            {
                ord = -1;
                continue;
            }

            long long pos;
            pos = dir->getPosition();

            if (FatFileSystem::isLongNameComponent(fcb))
            {
                if (fcb[LDIR_Ord] & LAST_LONG_ENTRY)
                {
                    ord = fcb[LDIR_Ord] & ~LAST_LONG_ENTRY;
                    if (0 < ord && ord <= 20)
                    {
                        sum = fcb[LDIR_Chksum];
                        l = FatFileSystem::assembleLongName(l, fcb);
                        --ord;
                    }
                    else
                    {
                        ord = -1;   // Orphan
                    }
                }
                else if (0 < ord && ord == fcb[LDIR_Ord] && sum == fcb[LDIR_Chksum])
                {
                    l = FatFileSystem::assembleLongName(l, fcb);
                    --ord;
                }
                else
                {
                    ord = -1;       // Orphan
                }
            }
            else if (!FatFileSystem::isVolumeID(fcb))
            {
                if (pos - 32 == offset)
                {
                    if (ord != 0 || sum != fileSystem->getChecksum(fcb))
                    {
                        // FCB without a long-name
                        l = longName;
                        if (!FatFileSystem::oemtoutf16(fcb, l))
                        {
                            break;
                        }
                    }
                    FatFileSystem::utf16toutf8(l, static_cast<char*>(name));    // XXX error check
                    return static_cast<char*>(name);
                    break;
                }
                ord = -1;
            }
            else
            {
                ord = -1;
            }
        }
    }
    return 0;
}
