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

    /**
     * Constructs a ring buffer object managing the specified ring buffer.
     * @param buf the ring buffer.
     * @param size the size of the ring buffer.
     */
    Ring(void* buf, long size) :
        buf(static_cast<u8*>(buf)), size(size), head(static_cast<u8*>(buf)), used(0)
    {
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
     * @param count     the length of the data in bytes.
     * @return          the length of the data discarded
     */
    long discard(long count);

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
