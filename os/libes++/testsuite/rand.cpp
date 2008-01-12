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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <es.h>

int main()
{
    srand48(0);

    printf("%lld\n\n", 1LL << 48);
    printf("%lld\n", rand48());
    printf("%lld\n", rand48());
    printf("%lld\n", rand48());
    printf("%lld\n", rand48());
    printf("%lld\n", rand48());
    printf("%lld\n", rand48());
    printf("%lld\n", rand48());
    printf("%lld\n", rand48());
    printf("%lld\n", rand48());
    printf("%lld\n", rand48());

    printf("done.\n");
}
