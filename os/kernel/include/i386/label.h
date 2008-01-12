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

#ifndef NINTENDO_ES_KERNEL_I386_LABEL_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_LABEL_H_INCLUDED

#include <es.h>

class Label
{
public:
    unsigned esp;       // 0
    unsigned eip;       // 4
    unsigned ebp;       // 8
    unsigned ebx;       // 12
    unsigned esi;       // 16
    unsigned edi;       // 20

    Label() :
        esp(0),
        eip(0),
        ebp(0),
        ebx(0),
        esi(0),
        edi(0)
    {
    }

    Label(void* stack, unsigned stackSize, void startUp(void* param), void* param) :
        ebp(0),
        ebx(0),
        esi(0),
        edi(0)
    {
        init(stack, stackSize, startUp, param);
    }

    void init(void* stack, unsigned stackSize, void startUp(void* param), void* param)
    {
        ASSERT(stackSize % sizeof(unsigned) == 0);

        unsigned* frame = static_cast<unsigned*>(stack);
        eip = (unsigned) startUp;
        esp = (unsigned) &frame[stackSize / sizeof(unsigned) - 2];
        frame[stackSize / sizeof(unsigned) - 2] = (unsigned) 0;
        frame[stackSize / sizeof(unsigned) - 1] = (unsigned) param;
    }

    int set();
    void jump();
};

#endif  // NINTENDO_ES_KERNEL_I386_LABEL_H_INCLUDED
