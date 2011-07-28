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

#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <es/types.h>
#include <es/formatter.h>

#ifndef NAN
#define NAN         __builtin_nan("")
#endif
#ifndef INFINITY
#define INFINITY    __builtin_inf()
#endif

using namespace es;

int print(const char* spec, ...)
{
    va_list list;

    va_start(list, spec);
    Formatter formatter((int (*)(int, void*)) putc, stdout);
    int count = formatter.format(spec, list);
    va_end(list);
    return count;
}

int main()
{
    u32 denf = 3;
    u64 den = 3;

    Formatter formatter((int (*)(int, void*)) putc, stdout);

    formatter.print(NAN);
    printf("\n");
    formatter.print(INFINITY);
    printf("\n\n");

    formatter.print(*(float*) &denf);
    printf("\n");
    formatter.print(*(double*) &den);
    printf("\n\n");

    formatter.print(FLT_MAX * FLT_MAX);
    printf("\n");
    formatter.print(DBL_MAX + DBL_MAX);
    printf("\n\n");

    formatter.print(0.0f);
    printf("\n");
    formatter.print(0.0);
    printf("\n\n");

    formatter.print(3.0f / 10.0f);
    printf("\n");
    formatter.print(3.0 / 10.0);
    printf("\n\n");

    formatter.print(3.14159265358979323846264338327f);
    printf("\n");
    formatter.print(3.14159265358979323846264338327);
    printf("\n\n");

    formatter.print(-1.0f / 3.0f);
    printf("\n");
    formatter.print(-1.0 / 3.0);
    printf("\n\n");

    formatter.print(powf(2.0f, 23.0f) * 2.5f);
    printf("\n");
    formatter.print(pow(2.0, 52.0) * 2.5);
    printf("\n\n");

    formatter.print(FLT_MAX);
    printf("\n");
    formatter.print(DBL_MAX);
    printf("\n\n");

    formatter.print(FLT_MIN);
    printf("\n");
    formatter.print(DBL_MIN);
    printf("\n\n");

    formatter.setBase(36);
    formatter.print(72 + 35);
    printf("\n\n");

    print("|%14s|\n", "hello");
    print("|%-14s|\n", "hello");
    print("|%14c|\n", 'A');
    print("|%#14x|\n", 0x1234);
    print("|%#14o|\n", 0123);
    print("Hello, world |%+05d| |%llu|\n", 100, 18446744073709551615ULL);
    print("|%14f|\n", 0.05);
    print("|%14f|\n", 1.05);
    print("|%14f|\n", 100.004);
    print("|%14.0e|\n", 2.3e+36);
    print("|%14e|\n", 1.4e-36);
    print("|%14g|\n", 0.0);
    print("|%14g|\n", 3.14);
    print("|%#14.6g|\n", 3.14);
    print("|%14g|\n", 300.0);
    print("|%14.12g|\n", 123456.789);
    print("|%#14.12g|\n", 123456.789);
    print("|%14.2g|\n", 3.14159265358979323846264338327);
    print("|%#14.1g|\n", 0.05);
    print("|%#14.2g|\n", 0.05);
    print("|%14g|\n", 1.4e+36);
    print("|%14g|\n", 1.4e-36);
    print("|%14.2g|\n", 0.000001234567);
    print("|%14g|\n", 2e-36);
    print("|%14g|\n", NAN);

    printf("\n");

    printf("|%14s|\n", "hello");
    printf("|%-14s|\n", "hello");
    printf("|%14c|\n", 'A');
    printf("|%#14x|\n", 0x1234);
    printf("|%#14o|\n", 0123);
    printf("Hello, world |%+05d| |%llu|\n", 100, 18446744073709551615ULL);
    printf("|%14f|\n", 0.05);
    printf("|%14f|\n", 1.05);
    printf("|%14f|\n", 100.004);
    printf("|%14.0e|\n", 2.3e+36);
    printf("|%14e|\n", 1.4e+36);
    printf("|%14e|\n", 1.4e-36);
    printf("|%14g|\n", 0.0);
    printf("|%14g|\n", 3.14);
    printf("|%#14.6g|\n", 3.14);
    printf("|%14g|\n", 300.0);
    printf("|%14.12g|\n", 123456.789);
    printf("|%#14.12g|\n", 123456.789);
    printf("|%14.2g|\n", 3.14159265358979323846264338327);
    printf("|%#14.1g|\n", 0.05);
    printf("|%#14.2g|\n", 0.05);
    printf("|%14g|\n", 1.4e+36);
    printf("|%14g|\n", 1.4e-36);
    printf("|%14.2g|\n", 0.000001234567);
    printf("|%14g|\n", 2e-36);
    printf("|%14g|\n", NAN);
}
