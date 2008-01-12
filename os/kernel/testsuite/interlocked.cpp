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

#include <stdlib.h>
#include <es.h>
#include <es/interlocked.h>

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    IInterface* nameSpace;
    esInit(&nameSpace);

    esReport("sizeof(char) = %u\n", sizeof(char));
    esReport("sizeof(short) = %u\n", sizeof(short));
    esReport("sizeof(int) = %u\n", sizeof(int));
    esReport("sizeof(long) = %u\n", sizeof(long));
    esReport("sizeof(long long) = %u\n", sizeof(long long));

    Interlocked count(0);
    esReport("count = %ld\n", static_cast<long>(count));
    TEST(count == 0);

    long n;
    n = count.exchange(3);
    esReport("count = %ld: n = %ld\n", static_cast<long>(count), n);
    TEST(count == 3 && n == 0);

    n = count.exchange(5);
    esReport("count = %ld: n = %ld\n", static_cast<long>(count), n);
    TEST(count == 5 && n == 3);

    n = count.increment();
    esReport("count = %ld: n = %ld\n", static_cast<long>(count), n);
    TEST(count == 6 && n == 6);

    n = count.decrement();
    esReport("count = %ld: n = %ld\n", static_cast<long>(count), n);
    TEST(count == 5 && n == 5);

    n = count.compareExchange(7, 3);
    esReport("count = %ld: n = %ld\n", static_cast<long>(count), n);
    TEST(count == 5 && n == 5);

    n = count.compareExchange(7, 5);
    esReport("count = %ld: n = %ld\n", static_cast<long>(count), n);
    TEST(count == 7 && n == 5);

    esReport("done.\n");
}
