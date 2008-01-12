/*
 * Copyright (c) 2007
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

#include <stdlib.h>

long long rand48(void)
{
    unsigned short xsubi[3];

    nrand48(xsubi);
    return ((long long) xsubi[2]) << 32 |
           ((long long) xsubi[1]) << 16 |
           ((long long) xsubi[0]);
}
