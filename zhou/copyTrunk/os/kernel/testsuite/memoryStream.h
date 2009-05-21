/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_KERNEL_TESTSUITE_MEMORYSTREAM_H_INCLUDED
#define NINTENDO_ES_KERNEL_TESTSUITE_MEMORYSTREAM_H_INCLUDED

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include <es/ref.h>
#include <es/interlocked.h>
#include <es/base/ICache.h>

class MemoryStream : public es::Stream
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

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::Stream::iid()) == 0)
        {
            objectPtr = static_cast<es::Stream*>(this);
        }
        else if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::Stream*>(this);
        }
        else
        {
            return NULL;
        }
        objectPtr->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
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
