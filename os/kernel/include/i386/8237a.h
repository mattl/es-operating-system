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

#ifndef NINTENDO_ES_KERNEL_I386_8237A_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_8237A_H_INCLUDED

#include <es/ref.h>
#include <es/types.h>
#include <es/device/IDmac.h>
#include "thread.h"

class Dmac
{
    static const u8 ADDR = 0;
    static const u8 COUNT = 1;
    static const u8 COMMAND = 8;
    static const u8 REQUEST = 9;
    static const u8 SINGLE_MASK = 10;
    static const u8 MODE = 11;
    static const u8 CLEAR_BYTE_POINTER = 12;
    static const u8 MASTER_CLEAR = 13;
    static const u8 CLEAR_MASK = 14;
    static const u8 ALL_MASK = 15;
    static const u8 pageOffset[4];

    Ref         ref;
    SpinLock    spinLock;
    u8          base;
    u8          page;
    int         shift;

public:


    class Chan : public IDmac
    {
        Dmac*   dmac;
        u8      chan;

    public:
        // IDmac
        void setup(void* addr, int count, u8 mode);
        void start();
        void stop();
        bool isDone();
        int getCount();

        // IInterface
        bool queryInterface(const Guid& riid, void** objectPtr);
        unsigned int addRef(void);
        unsigned int release(void);

        friend class Dmac;
    };


public:
    Dmac(u8 base, u8 page, int shift);
    void setup(u8 chan, u32 buffer, int len, u8 mode);

    Chan chan[4];

    friend class Chan;
};

#endif // NINTENDO_ES_KERNEL_I386_8237A_H_INCLUDED
