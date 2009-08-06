/*
 * Copyright 2008 Google Inc.
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

#include <ctype.h>
#include <stdio.h>

#include <es/types.h>

namespace es
{

void dump(const void* ptr, s32 len)
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

        fprintf(stderr, "%08x: ", (const u8*) ptr + j);
        for (i = 0; i < n; i++)
        {
            fprintf(stderr, "%02x ", ((const u8*) ptr)[j + i]);
        }

        for (; i < 16; i++)
        {
            fprintf(stderr, "   ");
        }

        fprintf(stderr, "  ");
        for (i = 0; i < n; i++)
        {
            fprintf(stderr, "%c", (isprint)(((const u8*) ptr)[j + i]) ? ((const u8*) ptr)[j + i] : '.');
        }

        fprintf(stderr, "\n");
    }
}

}   // namespace es
