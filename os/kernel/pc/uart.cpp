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

#include <es.h>
#include <es/clsid.h>
#include <es/synchronized.h>
#include "io.h"
#include "core.h"
#include "uart.h"

Uart::
Uart(int baseaddr, int bus, int irq) :
    baseaddr(baseaddr),
    ring(buffer, sizeof buffer)
{
    esReport("COM %x: ", baseaddr);

    outpb(baseaddr + FCR, 0xC7); // FIFO Control Register
    u8 x = inpb(baseaddr + IIR);
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

    if (0 <= irq)
    {
        interrupt = true;
        Core::registerInterruptHandler(bus, irq, this);
        outpb(baseaddr + IER, 1);   // Turn on receive interrupt
    }
    else
    {
        interrupt = false;
    }
}

Uart::
~Uart()
{
}

void Uart::
setBaud(int rate)
{
    // baud = 115200 / rate

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
    if (interrupt)
    {
        for (;;)
        {
            Monitor::Synchronized method(monitor);
            {
                Lock::Synchronized method(lock);
                n = ring.read(dst, count);
                if (0 < n)
                {
                    break;
                }
            }
            monitor.wait(10000);
        }
    }
    else
    {
        Lock::Synchronized method(lock);

        u8* ptr = static_cast<u8*>(dst);
        while (inpb(baseaddr + LSR) & 1)    // Check to see if char has been received.
        {
            *ptr++ = inpb(baseaddr);        // If so, then get Char
            ++n;
        }
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
        while (!(inpb(baseaddr + LSR) & (1<<5)))
        {
        }

        Lock::Synchronized method(lock);
        if (inpb(baseaddr + LSR) & (1<<5))
        {
            outpb(baseaddr, *ptr);
        }
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

int Uart::
invoke(int irq)
{
    if (!interrupt)
    {
        return 0;
    }

    {
        Lock::Synchronized method(lock);

        u8 x = inpb(baseaddr + IIR);
        switch (x & IIR_ID_MASK)
        {
          case IIR_MODEM_STATUS:
            break;
          case IIR_THR_EMPTY:
            break;
          case IIR_RECV_DATA:
          case IIR_CHAR_TIMEOUT:
            while (inpb(baseaddr + LSR) & 1)    // Check to see if char has been received.
            {
                u8 data = inpb(baseaddr);       // If so, then get Char
                if (0 < ring.getUnused())
                {
                    ring.write(&data, 1);
                }
            }
            break;
          case IIR_RECV_STATUS:
            break;
        }
    }
    monitor.notify();

    return 0;
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
addRef()
{
    return ref.addRef();
}

unsigned int Uart::
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
