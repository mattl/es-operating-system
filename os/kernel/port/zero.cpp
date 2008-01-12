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

#include "zero.h"

Zero::Zero()
{
}

Zero::~Zero()
{
}

long long Zero::getPosition()
{
    return 0;
}

void Zero::setPosition(long long pos)
{
}

long long Zero::getSize()
{
    return 9223372036854775807LL;   // 2^63-1
}

void Zero::setSize(long long size)
{
}

int Zero::read(void* dst, int count)
{
    return 0;
}

int Zero::read(void* dst, int count, long long offset)
{
    // Page::fill() will zero clear the page.
    return 0;
}

int Zero::write(const void* src, int count)
{
    return 0;
}

int Zero::write(const void* src, int count, long long offset)
{
    return 0;
}

void Zero::flush()
{
}

void* Zero::queryInterface(const Guid& riid)
{
    void* objectPtr;
    if (riid == IStream::iid())
    {
        objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IInterface::iid())
    {
        objectPtr = static_cast<IStream*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
}

unsigned int Zero::addRef(void)
{
    return ref.addRef();
}

unsigned int Zero::release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
