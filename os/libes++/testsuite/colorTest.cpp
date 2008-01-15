/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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
