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

#ifndef NINTENDO_ES_KERNEL_I386_CGA_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_CGA_H_INCLUDED

#include <es/types.h>
#include <es/ref.h>
#include <es/base/IStream.h>

using namespace es;

class Cga : public IStream
{
    static const u32 BASE = 0x800b8000;

    static const int CTL_ADDR = 0x3d4;      // Address register
    static const int CTL_DATA = 0x3d5;      // Data IO register
    static const u8 CURSOR_ADDR_HIGH = 0xe; // High byte of cursor address
    static const u8 CURSOR_ADDR_LOW = 0xf;  // Low  byte of cursor address

    Ref     ref;
    char*   cga;

    void putc(int c);

public:
    Cga();

    long long getPosition();
    void setPosition(long long pos);
    long long getSize();
    void setSize(long long size);
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();

    void* queryInterface(const Guid& riid);
    unsigned int addRef(void);
    unsigned int release(void);
};

#endif // NINTENDO_ES_KERNEL_I386_CGA_H_INCLUDED
