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

#include <es.h>
#include "io.h"
#include "uart.h"

Uart::
Uart(int baseaddr) :
    baseaddr(baseaddr)
{
    u8 x, olddata;

    esReport("COM %x: ", baseaddr);

    outpb(baseaddr + FCR, 0xC7); // FIFO Control Register
    x = inpb(baseaddr + IIR);
    switch (x & 0xc0)
    {
      case 0xc0:
        esReport("16550a\n");
        break;
      case 0x80:
        esReport("16550\n");
        break;
      case 0x00:
        esReport("16450\n");
        break;
    }

    outpb(baseaddr + IER, 0);       // Turn off interrupts - Port1
    outpb(baseaddr + LCR, 0x80);    // Set DLAB ON
    outpb(baseaddr + 0, 0x01);
    outpb(baseaddr + 1, 0x00);      // Set Baud rate - Divisor Latch High Byte
    outpb(baseaddr + LCR, 0x03);    // 8 Bits, No Parity, 1 Stop Bit
    outpb(baseaddr + FCR, 0xC7);    // FIFO Control Register
    outpb(baseaddr + MCR, 0x0B);    // Turn on DTR, RTS, and OUT2

    // outpb(baseaddr + IER, 1);    // Turn on receive interrupt
}

void Uart::
setBaud(int rate)
{
    outpb(baseaddr + LCR, 0x80);    // Set DLAB ON
    outpb(baseaddr + 0, rate);
    outpb(baseaddr + 1, 0x00);      // Set Baud rate - Divisor Latch High Byte
    outpb(baseaddr + LCR, 0x00);
}

long long Uart::
getPosition()
{
    return 0;
}

void Uart::
setPosition(long long pos)
{
}

long long Uart::
getSize()
{
    return 0;
}

void Uart::setSize(long long size)
{
}

int Uart::
read(void* dst, int count)
{
    int n = 0;

    u8* ptr = static_cast<u8*>(dst);
    while (inpb(baseaddr + LSR) & 1)    // Check to see if char has been received.
    {
        *ptr++ = inpb(baseaddr);        // If so, then get Char
        ++n;
    }
    return n;
}

int Uart::
read(void* dst, int count, long long offset)
{
    return read(dst, count);
}

int Uart::
write(const void* src, int count)
{
    int n;

    const u8* ptr = static_cast<const u8*>(src);
    for (n = 0; n < count; ++n, ++ptr)
    {
        // insert CR before LF.
        if (*ptr == '\n')
        {
            while (!(inpb(baseaddr + LSR) & (1<<5)))
            {
            }
            outpb(baseaddr, '\r');
        }

        while (!(inpb(baseaddr + LSR) & (1<<5)))
        {
        }
        outpb(baseaddr, *ptr);
    }
    return n;
}

int Uart::
write(const void* src, int count, long long offset)
{
    return write(src, count);
}

void Uart::
flush()
{
}

bool Uart::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IStream)
    {
        *objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IStream*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Uart::
addRef(void)
{
    return ref.addRef();
}

unsigned int Uart::
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
