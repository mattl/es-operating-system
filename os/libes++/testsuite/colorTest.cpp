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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <es/color.h>

int main()
{
    Rgb rgb;
    printf("%08x\n", (u32) rgb);

    rgb = Rgb("#123");
    printf("%08x\n", (u32) rgb);

    rgb = Rgb("#123456");
    printf("%08x\n", (u32) rgb);

    rgb = Rgb("rgb(16, 32, 48)");
    printf("%08x\n", (u32) rgb);

    rgb = Rgb("rgb(100%, 50%, 25%)");
    printf("%08x\n", (u32) rgb);

    rgb = Rgb("rgba(16, 32, 48, 0.5)");
    printf("%08x\n", (u32) rgb);

    rgb = Rgb("rgba(100%, 50%, 25%, 0.25)");
    printf("%08x\n", (u32) rgb);

    rgb = Rgb("red");
    printf("%08x\n", (u32) rgb);

    rgb = Rgb("green");
    printf("%08x\n", (u32) rgb);

    rgb = Rgb("blue");
    printf("%08x\n", (u32) rgb);

    try
    {
        rgb = Rgb("a"); // lower bound
    }
    catch (...)
    {
        printf("error: %08x\n", (u32) rgb);
    }

    try
    {
        rgb = Rgb("z"); // upper bound
    }
    catch (...)
    {
        printf("error: %08x\n", (u32) rgb);
    }

    rgb = "transparent";
    printf("%08x\n", (u32) rgb);

    rgb = 0x123456;
    printf("%08x\n", (u32) rgb);
}
