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
#include "cache.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

int main()
{
    IInterface* root;
    esInit(&root);

    unsigned addr;
    addr = 0x12345001;
    esReport("offset: %x\n", Page::pageOffset(addr));
    esReport("base: %x\n", Page::pageBase(addr));
    TEST(Page::pageOffset(addr) == 0x0001);
    TEST(Page::pageBase(addr) == 0x12345000);

    addr = 0x12345fff;
    esReport("offset: %x\n", Page::pageOffset(addr));
    esReport("base: %x\n", Page::pageBase(addr));
    TEST(Page::pageOffset(addr) == 0xfff);
    TEST(Page::pageBase(addr) == 0x12345000);

    void* ptr;
    ptr = (void*) 0x12345001;
    esReport("rount: %p\n", Page::round(ptr));
    esReport("trunc: %p\n", Page::trunc(ptr));
    TEST(Page::round(ptr) == (void*) 0x12346000);
    TEST(Page::trunc(ptr) == (void*) 0x12345000);

    esReport("done.\n");
}
