/*
 * Copyright 2008 Google Inc.
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

#ifndef NINTENDO_ES_RING_H_INCLUDED
#define NINTENDO_ES_RING_H_INCLUDED

#include <es.h>
#include <es/types.h>

/**
 * This class provides methods for managing a ring buffer.
 */
class Ring
{
    /** the ring buffer.
     */
    u8*     buf;
    /** the size of the ring buffer in bytes.
     */
    long    size;
    /** the head of data in the ring buffer.
     */
    u8*     head;
    /** the number of filled bytes in the ring buffer.
     */
    long    used;

    long pos(u8* org, u8* ptr)
    {
        return (org <= ptr) ? (ptr - org) : (ptr + size - org);
    }

public:
    struct Vec
    {
        void*   data;
        long    count;

        Vec() : data(0), count(0)
        {
        }
    };

    Ring() :
        buf(0),
        size(0),
        head(0),
        used(0)
    {
    }

    /**
     * Constructs a ring buffer object managing the specified ring buffer.
     * @param buf the ring buffer.
     * @param size the size of the ring buffer.
     */
    Ring(void* buf, long size) :
        buf(static_cast<u8*>(buf)),
        size(size),
        head(static_cast<u8*>(buf)),
        used(0)
    {
    }

    void initialize(void* buf, long size)
    {
        ASSERT(this->buf == 0);
        ASSERT(this->size == 0);

        this->buf = static_cast<u8*>(buf);
        this->size = size;
        this->head = static_cast<u8*>(buf);
        this->used = 0;
    }

    /** Peeks data from this ring buffer.
     * @param dst       the data to be peeked.
     * @param count     the length of the data in bytes.
     * @param offset    the position in the stored bytes from which the bytes is peeked.
     * @return          the length of the data peeked
     */
    long peek(void* dst, long count, long offset = 0) const;

    /** Reads data from this ring buffer.
     * @param dst       the data to be read.
     * @param count     the length of the data in bytes.
     * @return          the length of the data read
     */
    long read(void* dst, long count);

    /** Discards data from this ring buffer.
     * @param count     the number bytes to be discarded.
     * @return          the number bytes discarded
     */
    long skip(long count);

    /** Writes data to this ring buffer.
     * @param src       the data to be written.
     * @param count     the length of the data in bytes.
     * @return          the length of the data written
     */
    long write(const void* src, long count);

    /** Writes data to this ring buffer.
     * @param src       the data to be written.
     * @param count     the length of the data in bytes.
     * @param blocks    array of Vec{}.
     * @param maxblock  the number of Vec{} entries of the array.
     * @return          the length of the data written
     */
    long write(const void* src, long count, long offset, Vec* blocks, long maxblock);

    /**
     * Gets the number of filled bytes in this ring buffer.
     * @return the number of filled bytes.
     */
    long getUsed()
    {
        return used;
    }

    /**
     * Gets the number of non-filled bytes in this ring buffer.
     * @return the number of non-filled bytes.
     */
    long getUnused()
    {
        return size - used;
    }

    /**
     * Gets the size of this ring buffer.
     * @return the size of this ring buffer.
     */
    long getSize()
    {
        return size;
    }

    /**
     * Gets the head of data in this ring buffer.
     * @return the head of data.
     */
    u8* getHead()
    {
        return head;
    }

private:
    long marge(u8* adv, long count, Vec* blocks, long maxblock, u8* tail);
};

#endif  // NINTENDO_ES_RING_H_INCLUDED
