/*
 * Copyright (c) 2006, 2007
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

#ifndef NINTENDO_ES_KERNEL_I386_8259_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_8259_H_INCLUDED

#include <es/device/IPic.h>
#include <es/ref.h>

class Pic : public IPic
{
    Ref ref;
    u8  imrMaster;
    u8  imrSlave;

public:
    // Constructor
    Pic();

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);

    // IPic
    int startup(unsigned bus, unsigned irq);
    int shutdown(unsigned bus, unsigned irq);
    int enable(unsigned bus, unsigned irq);
    int disable(unsigned bus, unsigned irq);
    bool ack(int vec);
    bool end(int vec);
    int setAffinity(unsigned bus, unsigned irq, unsigned int cpuMask);
    unsigned int splIdle();
    unsigned int splLo();
    unsigned int splHi();
    void splX(unsigned int x);

    static const int PORT_MASTER =  0x20;
    static const int PORT_MASTER_IMR =  0x21;
    static const int PORT_SLAVE = 0xa0;
    static const int PORT_SLAVE_IMR = 0xa1;

    static const u8 OCW2 = 0x00;
    static const u8 OCW2_SPECIFIC_EOI = 0x60;
    static const u8 OCW3 = 0x08;
    static const u8 OCW3_ISR = OCW3 | 3;
    static const u8 OCW3_IRR = OCW3 | 2;
    static const u8 ICW1 = 0x10;

private:
    int readISR(unsigned irq);
};

#endif // NINTENDO_ES_KERNEL_I386_8259_H_INCLUDED
