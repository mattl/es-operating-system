/*
 * Copyright (c) 2006, 2007
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

//
// IBinding
//

#include <ctype.h>
#include <string.h>
#include <es.h>
#include <es/handle.h>
#include "fatStream.h"

IInterface* FatStream::
getObject()
{
    addRef();
    return static_cast<IContext*>(this);
}

int FatStream::
setObject(IInterface* object)
{
    return -1;
}

int FatStream::
getName(char* name, unsigned int len)
{
    Synchronized<IMonitor*> method(monitor);

    int ord = -1;   // The order of long-name entry
    u8 sum;
    u16 longName[13 * 20 + 1];
    u16* l;

    if (1 <= len)
    {
        name[0] = 0;
    }

    if (isRoot() || isRemoved())
    {
        return 0;
    }

    // Clear the directory entry including the long name entries
    Handle<IStream> dir(parent->cache->getStream());
    {
        Synchronized<IMonitor*> method(parent->monitor);

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
                    FatFileSystem::utf16toutf8(l, name);    // XXX error check
                    return strlen(name);                    // XXX for esjs
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
