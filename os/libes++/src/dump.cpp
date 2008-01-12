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

#include <ctype.h>
#include <es.h>

void esDump(const void* ptr, s32 len)
{
    int j;
    int i;
    int n;

    for (j = 0; j < len; j += 16)
    {
        n = len - j;
        if (16 < n)
        {
            n = 16;
        }

        esReport("%08x: ", (const u8*) ptr + j);
        for (i = 0; i < n; i++)
        {
            esReport("%02x ", ((const u8*) ptr)[j + i]);
        }

        for (; i < 16; i++)
        {
            esReport("   ");
        }

        esReport("  ");
        for (i = 0; i < n; i++)
        {
            esReport("%c", (isprint)(((const u8*) ptr)[j + i]) ? ((const u8*) ptr)[j + i] : '.');
        }

        esReport("\n");
    }
}
