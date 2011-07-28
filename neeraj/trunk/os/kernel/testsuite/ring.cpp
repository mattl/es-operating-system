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

#include <es.h>
#include <es/ring.h>
#include <es/naming/IContext.h>
#include <string.h>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main(void)
{
    u8 i, j, k, l, m, n, o;
    Ring::Vec blocks[5];
    u8 buf[5];
    u8 data[5];
    u8 pat[5];

    Object* nameSpace = 0;
    esInit(&nameSpace);

    for (i = 0; i < 5; ++i)
    {
        for (j = 0; j < 5; ++j)
        {
            if (j == i)
            {
                continue;
            }
            for (k = 0; k < 5; ++k)
            {
                if (k == j || k == i)
                {
                    continue;
                }
                for (l = 0; l < 5; ++l)
                {
                    if (l == k || l == j || l == i)
                    {
                        continue;
                    }
                    for (m = 0; m < 5; ++m)
                    {
                        if (m == l || m == k || m == j || m == i)
                        {
                            continue;
                        }

                        esReport("%d%d%d%d%d\n", i, j, k, l, m);

                        pat[0] = i;
                        pat[1] = j;
                        pat[2] = k;
                        pat[3] = l;
                        pat[4] = m;

                        memset(blocks, 0, sizeof blocks);
                        memset(buf, '-', 5);

                        for (n = 0; n < 5; ++n)
                        {
                            Ring ring(buf, 5);

                            for (o = 0; o < 5; ++o)
                            {
                                long offset = pat[o] - ring.getUsed();
                                if (offset < 0)
                                {
                                    offset += 5;
                                }
                                ring.write((u8*) &"01234"[pat[o]], 1, offset,
                                           blocks, 5);
                            }

                            long used = ring.getUsed();
                            TEST(used == 5);
                            ring.read(data, used);
                            TEST(memcmp(data, "01234", used) == 0);
                        }
                    }
                }
            }
        }
    }

    memset(blocks, 0, sizeof blocks);
    memset(buf, '-', 5);
    Ring ring(buf, 5);

    long count = 2;
    ring.write((u8*) "34", count, 3, blocks, 5);
    long used = ring.getUsed();
    TEST(used == 0);

    count = 3;
    ring.write((u8*) "012", count, 0, blocks, 5);

    used = ring.getUsed();
    TEST(used == 5);
    ring.read(data, used);
    TEST(memcmp(data, "01234", used) == 0);

    ring.write("abc", 3);
    used = ring.getUsed();
    TEST(used == 3);
    ring.read(data, used);
    TEST(memcmp(data, "abc", used) == 0);

    ring.write("defgh", 5);
    used = ring.getUsed();
    TEST(used == 5);
    ring.read(data, used);
    TEST(memcmp(data, "defgh", used) == 0);

    esReport("done.\n");
}
