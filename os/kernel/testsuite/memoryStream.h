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

#ifndef NINTENDO_ES_KERNEL_TESTSUITE_MEMORYSTREAM_H_INCLUDED
#define NINTENDO_ES_KERNEL_TESTSUITE_MEMORYSTREAM_H_INCLUDED

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include <es/ref.h>
#include <es/clsid.h>
#include <es/interlocked.h>
#include <es/base/ICache.h>

class MemoryStream : public IStream
{
    Ref     ref;
    size_t  size;
    void*   store;

public:
    MemoryStream(size_t size = 0) :
        size(size),
        store(0)
    {
        if (0 < size)
        {
            store = malloc(size);
            if (!store)
            {
                esThrow(ENOSPC);
            }
        }
    }

    ~MemoryStream()
    {
        if (0 < size)
        {
            free(store);
        }
    }

    long long getPosition()
    {
        return 0;
    }

    void setPosition(long long pos)
    {
    }

    long long getSize()
    {
        return this->size;
    }

    void setSize(long long size)
    {
        if (size == this->size)
        {
            return;
        }
        if (size == 0)
        {
            free(store);
            store = 0;
        }
        else
        {
            void* ptr = realloc(store, size);
            if (!ptr)
            {
                esThrow(ENOSPC);
            }
            store = ptr;
        }
        this->size = size;
    }

    int read(void* dst, int count)
    {
        return read(dst, count, 0);
    }

    int read(void* dst, int count, long long offset)
    {
#ifdef VERBOSE
        esReport("MemoryStream::read %ld byte from %lld.\n", count, offset);
#endif
        if (size < offset || count < 0)
        {
            esThrow(EINVAL);
        }
        if (size - offset < count)
        {
            count = size - offset;
        }
        if (count == 0)
        {
            return 0;
        }
        memmove(dst, (u8*) store + offset, count);
        return count;
    }

    int write(const void* src, int count)
    {
        return write(src, count, 0);
    }

    int write(const void* src, int count, long long offset)
    {
#ifdef VERBOSE
        esReport("MemoryStream::write %ld byte from %lld.\n", count, offset);
#endif
        if (size < offset || count < 0)
        {
            esThrow(EINVAL);
        }
        if (size - offset < count)
        {
            count = size - offset;
        }
        if (count == 0)
        {
            return 0;
        }
        memmove((u8*) store + offset, src, count);
        return count;
    }

    void flush()
    {
    }

    bool queryInterface(const Guid& riid, void** objectPtr)
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

    unsigned int addRef(void)
    {
        return ref.addRef();
    }

    unsigned int release(void)
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
        }
        return count;
    }
};

#endif // NINTENDO_ES_KERNEL_TESTSUITE_MEMORYSTREAM_H_INCLUDED
