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

#include <stdio.h>
#include <es.h>
#include <es/utf.h>
#include "cpu.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    IInterface* root = NULL;
    esInit(&root);

    __asm__ __volatile__ ("cli\n");

    Xreg xreg;
    xreg.restore();
    xreg.save();
    xreg.dump();

    double a = 3.5;
    double b = 4.9;
    esReport("%f\n", a + b);
    TEST(a + b == 8.4);

    esReport("done.\n");
}
