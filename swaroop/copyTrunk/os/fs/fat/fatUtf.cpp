/*
 * Copyright 2008 Google Inc.
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
#include <errno.h>
#include <string.h>
#include <es.h>
#include "fatStream.h"

//
// Character set
//

const u8 FatFileSystem::
nameDot[11] =
{
    '.', 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

const u8 FatFileSystem::
nameDotdot[11] =
{
    '.', '.', 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

bool FatFileSystem::
isDelimitor(int c)
{
    return (c == '/' || c == '\\') ? true : false;
}

bool FatFileSystem::
isValidShortChar(u8 ch)
{
    static u32 map[] =
    {
                    //            0123456789abcdef 0123456789abcdef
        0x00000000, // 0x00-0x1f:
                    // 0x20-0x3f: 1101111111000110 1111111111000000
        0xdfc6ffc0, // 0x20-0x3f: _! #$%&'() +,-.  0123456789 ; =
                    // 0x40-0x5f: 1111111111111111 1111111111100011
        0xffffffe3, // 0x40-0x5f: @ABCDEFGHIJKLMNO PQRSTUVWXYZ[ ]^_
                    // 0x60-0x7f: 1000000000000000 0000000000010110
        0x80000016, // 0x60-0x7f: `                           { }~
        0x00000000, // 0x80-0x9f: 0000000000000000 0000000000000000
        0x00000000, // 0xa0-0xbf: 0000000000000000 0000000000000000
        0x00000000, // 0xc0-0xdf: 0000000000000000 0000000000000000
        0x00000000  // 0xe0-0xff: 0000000000000000 0000000000000000
    };
    return (map[ch / 32] & (0x80000000u >> (ch % 32))) != 0;
}

bool FatFileSystem::
isValidLongChar(u8 ch)
{
    static u32 map[] =
    {
                    //            0123456789abcdef 0123456789abcdef
        0x00000000, // 0x00-0x1f:
                    // 0x20-0x3f: 1101111111011110 1111111111010100
        0xdfdeffd4, // 0x20-0x3f: _! #$%&'() +,-.  0123456789 ; =
                    // 0x40-0x5f: 1111111111111111 1111111111110111
        0xfffffff7, // 0x40-0x5f: @ABCDEFGHIJKLMNO PQRSTUVWXYZ[ ]^_
                    // 0x60-0x7f: 1000000000000000 0000000000010110
        0x80000016, // 0x60-0x7f: `                           { }~
        0x00000000, // 0x80-0x9f: 0000000000000000 0000000000000000
        0x00000000, // 0xa0-0xbf: 0000000000000000 0000000000000000
        0x00000000, // 0xc0-0xdf: 0000000000000000 0000000000000000
        0x00000000  // 0xe0-0xff: 0000000000000000 0000000000000000
    };
    return (map[ch / 32] & (0x80000000u >> (ch % 32))) != 0;
}

bool FatFileSystem::
oemtoutf16(const u8* fcb, u16* utf16)
{
    int i, j;

    // main part
    for (j = 7; 0 <= j && fcb[j] == 0x20; --j)
    {
        ;
    }
    if (j < 0)
    {
        return false;
    }
    for (i = 0; i <= j; ++i)
    {
        if (!isValidShortChar(fcb[i]))
        {
            return false;
        }
        if (fcb[DIR_NTRes] & NTRes_LOWER8)
        {
            *utf16++ = tolower(fcb[i]);
        }
        else
        {
            *utf16++ = fcb[i];
        }
    }

    for (j = 10; 8 <= j && fcb[j] == 0x20; --j)
    {
        ;
    }
    if (8 <= j)
    {
        // dot
        *utf16++ = 0x2e;

        // extention
        for (i = 8; i <= j; ++i)
        {
            if (!isValidShortChar(fcb[i]))
            {
                return false;
            }
            if (fcb[DIR_NTRes] & NTRes_LOWER3)
            {
                *utf16++ = tolower(fcb[i]);
            }
            else
            {
                *utf16++ = fcb[i];
            }
        }
    }

    *utf16++ = 0;
    return true;
}

// Assum utf16 is a valid long name.
u8 FatFileSystem::
oemCode(u16 utf16, bool& lossy)
{
    u8 ch;

    if (utf16 == 0)
    {
        return 0;
    }
    else if (utf16 < 0x7f)
    {
        ch = (u8) toupper(utf16);
        if (!isValidLongChar(ch))
        {
            esThrow(EACCES);
        }
        if (!isValidShortChar(ch))
        {
            ch = 0x5f;  // '_'
            lossy = true;
        }
    }
    else
    {
        ch = 0x5f;  // '_'
        lossy = true;
    }
    return ch;
}

bool FatFileSystem::
utf16tooem(const u16* utf16, u8* fcb)
{
    bool lossy = false;
    int case8 = 0;
    int case3 = 0;
    u8 ch;
    u8* oem = fcb;

    fcb[DIR_NTRes] = 0;

    // main part
    int n = 0;
    while ((ch = oemCode(*utf16, lossy)) != 0 &&
           // ch != 0x2e /* '.' */ &&
           n < 8)
    {
        if (ch == 0x20)
        {
            lossy = true;
        }
        else
        {
            if (ch == 0x2e /* '.' */)
            {
                if (fcb == oem)
                {
                    ch = 0x5f;  // '_'
                    lossy = true;
                }
                else
                {
                    break;
                }
            }

            if (!lossy)
            {
                if (isupper(*utf16))
                {
                    if (case8 <= 0)
                    {
                        case8 = -1;
                    }
                    else
                    {
                        lossy = true;
                    }
                }
                if (islower(*utf16))
                {
                    if (0 <= case8)
                    {
                        case8 = 1;
                    }
                    else
                    {
                        lossy = true;
                    }
                }
            }
            *oem++ = ch;
            ++n;
        }

        ++utf16;
    }
    while (n++ < 8)
    {
        *oem++ = 0x20;
    }

    // extention part
    n = 0;
    u8* ext = oem;
    while (ch)
    {
        if (ch != 0x2e /* '.' */)
        {
            lossy = true;
            while ((ch = oemCode(*utf16++, lossy)) != 0 && ch != 0x2e /* '.' */)
            {
                ;
            }
        }

        if (ch == 0x2e /* '.' */)
        {
            if (0 < n)
            {
                lossy = true;
            }
            n = 0;
            oem = ext;
            while ((ch = oemCode(*utf16, lossy)) != 0 &&
                   ch != 0x2e /* '.' */ &&
                   n < 3)
            {
                if (ch == 0x20)
                {
                    lossy = true;
                }
                else
                {
                    *oem++ = ch;
                    ++n;
                }

                if (!lossy)
                {
                    if (isupper(*utf16))
                    {
                        if (case3 <= 0)
                        {
                            case3 = -1;
                        }
                        else
                        {
                            lossy = true;
                        }
                    }
                    if (islower(*utf16))
                    {
                        if (0 <= case3)
                        {
                            case3 = 1;
                        }
                        else
                        {
                            lossy = true;
                        }
                    }
                }
                ++utf16;
            }
            ++utf16;
        }
    }
    while (n++ < 3)
    {
        *oem++ = 0x20;
    }

    if (!lossy)
    {
        ++oem;  // Skip DIR_Attr
        // Set DIR_NTRes if necessary
        if (0 < case8)
        {
            *oem |= NTRes_LOWER8;
        }
        if (0 < case3)
        {
            *oem |= NTRes_LOWER3;
        }
    }

    return lossy;
}

// XXX error check
void FatFileSystem::
utf16toutf8(const u16* utf16, char* utf8)
{
    do {
        u32 utf32;
        utf16 = utf16to32(utf16, &utf32);
        utf8 = utf32to8(utf32, utf8);
    } while (utf16 && *utf16);
    *utf8 = 0;
}

void FatFileSystem::
utf8toutf16(const char* utf8, u16* utf16)
{
    const u16* limit = &utf16[255];

    do {
        u32 utf32;
        utf8 = utf8to32(utf8, &utf32);
        utf16 = utf32to16(utf32, utf16);
    } while (utf8 && *utf8 && utf16 < limit);
    if (!utf8 || *utf8)
    {
        esThrow(ENAMETOOLONG);
    }
    *utf16 = 0;
}

// Get the first pathname component from utf8 to utf16.
const char* FatFileSystem::
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

// Compare fileName with fcbName and the OEM name in fcb.
bool FatFileSystem::
isEqual(const u16* fileName, const u16* fcbName, const u8* fcb)
{
    u16 oemName[24];

    if (utf16icmp(fileName, fcbName) == 0)
    {
        return true;
    }
    if (FatFileSystem::oemtoutf16(fcb, oemName) &&
        utf16icmp(fileName, oemName) == 0)
    {
        return true;
    }
    return false;
}

// Assembles the long-name from long-name subcomponents.
u16* FatFileSystem::
assembleLongName(u16* longName, u8* fcb)
{
    int i;

    for (i = 2; 0 <= i; i -= 2)
    {
        *--longName = word(fcb + LDIR_Name3 + i);
    }
    for (i = 10; 0 <= i; i -= 2)
    {
        *--longName = word(fcb + LDIR_Name2 + i);
    }
    for (i = 8; 0 <= i; i -= 2)
    {
        *--longName = word(fcb + LDIR_Name1 + i);
    }
    return longName;
}

// Fills a part of this long-name to this FCB.
void FatFileSystem::
fillLongName(u8* fcb, const u16* longName, int ord)
{
    int i;
    bool terminated = false;

    longName += ((ord & ~LAST_LONG_ENTRY) - 1) * 13;
    for (i = 0; i <= 8; i += 2)
    {
        if (terminated)
        {
            xword(fcb + LDIR_Name1 + i, 0xffff);
        }
        else
        {
            xword(fcb + LDIR_Name1 + i, *longName);
            if (*longName == 0)
            {
                terminated = true;
            }
            ++longName;
        }
    }
    for (i = 0; i <= 10; i += 2)
    {
        if (terminated)
        {
            xword(fcb + LDIR_Name2 + i, 0xffff);
        }
        else
        {
            xword(fcb + LDIR_Name2 + i, *longName);
            if (*longName == 0)
            {
                terminated = true;
            }
            ++longName;
        }
    }
    for (i = 0; i <= 2; i += 2)
    {
        if (terminated)
        {
            xword(fcb + LDIR_Name3 + i, 0xffff);
        }
        else
        {
            xword(fcb + LDIR_Name3 + i, *longName);
            if (*longName == 0)
            {
                terminated = true;
            }
            ++longName;
        }
    }
}

int FatFileSystem::
getNumericTrail(const u8*& oem)
{
    // Move s to the last non-space base name character.
    const u8* s = oem + 7;
    while (*s == 0x20)
    {
        --s;
    }
    const u8* end = s;

    int n = 0;
    int d = 0;
    int k = 1;
    while (oem < s)
    {
        if (isdigit(*s))
        {
            d = *s - '0';
            n += d * k;
            k *= 10;
        }
        else if (*s == '~')
        {
            if (d == 0)
            {
                break;
            }
            oem = s - 1;
            return n;
        }
        else
        {
            break;
        }
        --s;
    }

    oem = end;
    return 0;
}

// Returns the numeric trail value of fcb.
int FatFileSystem::
getNumericTrail(const u8* oem, const u8* fcb)
{
    // Compare extensions
    if (memcmp(oem + 8, fcb + 8, 3) != 0)
    {
        return 0;
    }

    const u8* s2 = fcb;
    int n2 = getNumericTrail(s2);
    if (n2 == 0)
    {
        return 0;
    }
    int o2 = s2 - fcb;

    const u8* s1 = oem;
    getNumericTrail(s1);
    int o1 = s1 - oem;

    if (o1 < o2)
    {
        if (oem[7] == ' ')
        {
            // o2 is too long.
            return 0;
        }
        s2 = fcb + o1;
    }
    else if (o2 < o1)
    {
        if (fcb[7] == ' ')
        {
            // o1 is too long.
            return 0;
        }
        s1 = oem + o2;
    }

    while (fcb <= s2)
    {
        if (*s1-- != *s2--)
        {
            return 0;
        }
    }

    return n2;
}

void FatFileSystem::
setNumericTrail(u8* oem, int n)
{
    ASSERT(0 < n && n < 1000000);

    // Set k to the number of digits that represents n.
    int d;
    int k;
    for (k = 0, d = n; d; d /= 10)
    {
        ++k;
    }

    const u8* s = oem;
    getNumericTrail(s);

    // Move s to the new end of the base name.
    s += 1 + k;
    if (oem + 7 < s)
    {
        s = oem + 7;
    }

    // Fill in the numeric trail.
    u8* t = const_cast<u8*>(s);
    while (n)
    {
        *t-- = (u8) (0x30 + (n % 10));
        n /= 10;
    }
    *t = 0x7e /* '~' */;
}
