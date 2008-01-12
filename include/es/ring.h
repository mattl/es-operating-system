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

#ifndef NINTENDO_ES_RING_H_INCLUDED
#define NINTENDO_ES_RING_H_INCLUDED

#include <es/types.h>

class Ring
{
    u8*     buf;    // Pointer to the ring buffer
    long    size;   // The size of the ring buffer in bytes
    u8*     head;   // Pointer to the head of data in the ring buffer
    long    used;   // The number of filled bytes in the ring buffer

    long min(long a, long b)
    {
        return (a < b) ? a : b;
    }

    long max(long a, long b)
    {
        return (b < a) ? a : b;
    }

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

    Ring(void* buf, long size) :
        buf(static_cast<u8*>(buf)), size(size), head(static_cast<u8*>(buf)), used(0)
    {
    }

    /** Peeks data from the ring buffer.
     * @param dst       pointer to data to be peeked.
     * @param count     the length of the data in bytes.
     * @return          the length of the data peeked
     */
    long peek(void* dst, long count);

    /** Reads data from the ring buffer.
     * @param dst       pointer to data to be read.
     * @param count     the length of the data in bytes.
     * @return          the length of the data read
     */
    long read(void* dst, long count);

    /** Writes data to the ring buffer.
     * @param src       pointer to data to be written.
     * @param count     the length of the data in bytes.
     * @return          the length of the data written
     */
    long write(const void* src, long count);

    /** Writes data to the ring buffer.
     * @param src       pointer to data to be written.
     * @param count     the length of the data in bytes.
     * @param blocks    array of Vec{}.
     * @param maxblock  the number of Vec{} entries of the array.
     * @return          the length of the data written
     */
    long write(const void* src, long count, long offset, Vec* blocks, long maxblock);

    long getUsed()
    {
        return used;
    }

private:
    long marge(u8* adv, long count, Vec* blocks, long maxblock, u8* tail);
};

#endif  // NINTENDO_ES_RING_H_INCLUDED
