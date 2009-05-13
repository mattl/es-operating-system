/*
 * Copyright 2008, 2009 Google Inc.
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

Object* Pic::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Pic::iid()) == 0)
    {
        objectPtr = static_cast<es::Pic*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Pic*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Pic::
addRef()
{
    return ref.addRef();
}

unsigned int Pic::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
