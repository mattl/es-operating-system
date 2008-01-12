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

#ifndef NINTENDO_ES_KERNEL_UART_H_INCLUDED
#define NINTENDO_ES_KERNEL_UART_H_INCLUDED

#include <es/types.h>
#include <es/ref.h>
#include <es/base/IStream.h>
#include "spinlock.h"

class Uart : public IStream
{
    static const u8 IER = 1;
    static const u8 IIR = 2;   // read
    static const u8 FCR = 2;   // write
    static const u8 LCR = 3;
    static const u8 MCR = 4;
    static const u8 LSR = 5;
    static const u8 MSR = 6;

    Ref  ref;
    int  baseaddr;
    Lock lock;

public:
    Uart(int baseaddr);

    // Set Baud rate - Divisor Latch Low Byte
    // Default 0x03 = 38,400 BPS
    // 0x01 = 115,200 BPS
    // 0x02 = 56,700 BPS
    // 0x06 = 19,200 BPS
    // 0x0C = 9,600 BPS
    // 0x18 = 4,800 BPS
    // 0x30 = 2,400 BPS
    void setBaud(int rate);

    long long getPosition();
    void setPosition(long long pos);
    long long getSize();
    void setSize(long long size);
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();

    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);
};

#endif // NINTENDO_ES_KERNEL_UART_H_INCLUDED
