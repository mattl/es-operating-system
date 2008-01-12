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

#ifndef __WIN32__

#include <ctype.h>
#include <string.h>
#include "core.h"

size_t strnlen(const char *s, const size_t n)
{
    int i;
    const char* t;

    for (i = 0, t = s; i < n && *t != '\0'; ++i, ++t)
    {

    }
    return (size_t) (t - s);
}

int stricmp(const char *s1, const char *s2)
{
    char c1;
    char c2;

    for (;;)
    {
        c1 = (char) (tolower)(*s1++);
        c2 = (char) (tolower)(*s2++);
        if (c1 < c2)
        {
            return -1;
        }
        if (c1 > c2)
        {
            return 1;
        }
        if (c1 == '\0')
        {
            return 0;
        }
    }
    return 0;
}

int strnicmp(const char *s1, const char *s2, size_t n)
{
    int i;
    char c1;
    char c2;

    for (i = 0; i < n; ++i)
    {
        c1 = (char) (tolower)(*s1++);
        c2 = (char) (tolower)(*s2++);
        if (c1 < c2)
        {
            return -1;
        }
        if (c1 > c2)
        {
            return 1;
        }
        if (c1 == '\0')
        {
            return 0;
        }
    }
    return 0;
}

#endif // __WIN32__
