/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

#include <stdlib.h>
#include <es.h>
#include <es/interlocked.h>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    Object* nameSpace;
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
