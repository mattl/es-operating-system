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

// 8259 interrupt controllers

#include <es.h>
#include "io.h"
#include "8259.h"

Pic::
Pic() :
    ref(1), imrMaster(0xff), imrSlave(0xff)
{
    outpb(PORT_MASTER_IMR, imrMaster);  // mask all master interrupts
    outpb(PORT_SLAVE_IMR, imrSlave);    // mask all slave interrupts

    outpb(PORT_MASTER, ICW1 | 0x01);    // master ICW1
    outpb(PORT_MASTER_IMR, 0x20);       // master ICW2 (interrupt vector base address)
    outpb(PORT_MASTER_IMR, 0x04);       // master ICW3 (IR2 has a slave)
    outpb(PORT_MASTER_IMR, 0x01);       // master ICW4 (8086 mode)

    outpb(PORT_SLAVE, ICW1 | 0x01);     // slave ICW1
    outpb(PORT_SLAVE_IMR, 0x28);        // slave ICW2 (interrupt vector base address)
    outpb(PORT_SLAVE_IMR, 0x02);        // slave ICW3 (connected to IR2 of the master)
    outpb(PORT_SLAVE_IMR, 0x01);        // slave ICW4 (8086 mode)

    // Pass #2 8259 interrupts to #1
    imrMaster &= ~04;
    outpb(PORT_MASTER_IMR, imrMaster);
}

int Pic::
startup(unsigned bus, unsigned irq)
{
    return 32 + irq;
}

int Pic::
shutdown(unsigned bus, unsigned irq)
{
    return disable(bus, irq);
}

int Pic::
enable(unsigned bus, unsigned irq)
{
    if (irq & 8)
    {
        imrSlave &= ~(1u << (irq & 7));
        outpb(PORT_SLAVE_IMR, imrSlave);
    }
    else
    {
        imrMaster &= ~(1u << (irq & 7));
        outpb(PORT_MASTER_IMR, imrMaster);
    }
    return 32 + irq;
}

int Pic::
disable(unsigned bus, unsigned irq)
{
    if (irq & 8)
    {
        imrSlave |= (1u << (irq & 7));
        outpb(PORT_SLAVE_IMR, imrSlave);
    }
    else
    {
        imrMaster |= (1u << (irq & 7));
        outpb(PORT_MASTER_IMR, imrMaster);
    }
    return 32 + irq;
}

int Pic::
readISR(unsigned irq)
{
    int isr;
    if (irq & 8)
    {
        outpb(PORT_SLAVE, OCW3_ISR);
        isr = inpb(PORT_SLAVE);
        outpb(PORT_SLAVE, OCW3_IRR);
    }
    else
    {
        outpb(PORT_MASTER, OCW3_ISR);
        isr = inpb(PORT_MASTER);
        outpb(PORT_MASTER, OCW3_IRR);
    }
    return isr;
}

bool Pic::
ack(int vec)
{
    ASSERT(32 <= vec && vec < 32 + 16);
    unsigned irq = vec - 32;
    unsigned irqmask = 1u << (irq & 7);
    if (readISR(irq) & irqmask)
    {
        if (irq & 8)
        {
            imrSlave |= irqmask;
            outpb(PORT_SLAVE_IMR, imrSlave);
            outpb(PORT_SLAVE, OCW2_SPECIFIC_EOI + (irq & 7));
            outpb(PORT_MASTER, OCW2_SPECIFIC_EOI + 2);
        }
        else
        {
            imrMaster |= irqmask;
            outpb(PORT_MASTER_IMR, imrMaster);
            outpb(PORT_MASTER, OCW2_SPECIFIC_EOI + irq);
        }
        return true;
    }
    return false;
}

bool Pic::
end(int vec)
{
    enable(0, vec - 32);
    return true;
}

int Pic::
setAffinity(unsigned bus, unsigned irq, unsigned int cpuMask)
{
    return 32 + irq;
}

unsigned Pic::
splIdle()
{
    register unsigned eax;

    __asm__ __volatile__ (
        "pushfl\n"
        "popl    %0\n"
        "sti"
        : "=a" (eax));

    return eax;
}

unsigned Pic::
splLo()
{
    register unsigned eax;

    __asm__ __volatile__ (
        "pushfl\n"
        "popl   %0\n"
        "sti"
        : "=a" (eax));

    return eax;
}

unsigned Pic::
splHi()
{
    register unsigned eax;

    __asm__ __volatile__ (
        "pushfl\n"
        "popl   %0\n"
        "cli"
        : "=a" (eax));

    return eax;
}

void Pic::
splX(unsigned x)
{
    __asm__ __volatile__ (
        "pushl   %0\n"
        "popfl"
        :: "r" (x));
}

//
// IInterface
//

void* Pic::
queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == IPic::iid())
    {
        objectPtr = static_cast<IPic*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<IPic*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
}

unsigned int Pic::
addRef(void)
{
    return ref.addRef();
}

unsigned int Pic::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
