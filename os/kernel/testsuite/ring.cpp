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

    IInterface* nameSpace = 0;
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
                            TEST(ring.write("01234", n) == n);
                            TEST(ring.read(data, n) == n);
                            TEST(memcmp(data, "01234", n) == 0);

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

    long offset = 2;
    ring.write((u8*) "34", offset, 3, blocks, 5);

    offset = 3;
    ring.write((u8*) "012", offset, 0, blocks, 5);

    long used = ring.getUsed();
    TEST(used == 5);
    ring.read(data, used);
    TEST(memcmp(data, "01234", used) == 0);

    esReport("done.\n");
}
