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

#ifndef NINTENDO_ES_KERNEL_ARENA_H_INCLUDED
#define NINTENDO_ES_KERNEL_ARENA_H_INCLUDED

//
//                       Arena
//                      +--------+
//    +-----------------|  head  |
//    |                 +--------+
//    |                 |  tail  |-------------------+
//    |                 +--------+                   |
//    |                                              |
//    |                                              |
//    |   Cell           Cell           Cell         |
//    |  +-+-+-+----+   +-+-+-+----+   +-+-+-+----+  |
//    +->|p|n|s|    |---|p|n|s|    |---|p|n|s|    |<-+
//       |r|e|i|    |   |r|e|i|    |   |r|e|i|    |
//       |e|x|z|    |   |e|x|z|    |   |e|x|z|    |
//       |v|t|e|    |   |v|t|e|    |   |v|t|e|    |
//       +-+-+-+----+   +-+-+-+----+   +-+-+-+----+
//

#include <cstddef>
#include <es/list.h>
#include "thread.h"

class Arena
{
public:
    static const size_t ALIGN = 32;         // Pentium 4 (L1)
    static const size_t SHIFT = 5;

    void* alloc(size_t size, size_t align) throw();
    void* allocLast(size_t size, size_t align) throw();
    void free(void* place, size_t size) throw();
    size_t size() throw();

    static char* trunc(const void* ptr, size_t align) throw()
    {
        size_t addr = reinterpret_cast<size_t>(ptr);
        return reinterpret_cast<char*>(addr & ~(align - 1));
    }

    static char* round(const void* ptr, size_t align) throw()
    {
        size_t addr = reinterpret_cast<size_t>(ptr);
        return reinterpret_cast<char*>((addr + align - 1) & ~(align - 1));
    }

    struct Cell
    {
        Link<Cell>  link;
        size_t      size;

        typedef List<Cell, &Cell::link> List;

        Cell(size_t size) throw() :
            size(size)
        {
        };

        const char* left() const throw()
        {
            return reinterpret_cast<const char*>(this);
        };

        const char* right() const throw()
        {
            return left() + size;
        };
    };

    Arena() throw()
    {
    }

    Arena(void* place, size_t size) throw()
    {
        free(place, size);
    }

private:
    Lock        spinLock;
    Cell::List  unused;
};

#endif // NINTENDO_ES_KERNEL_ARENA_H_INCLUDED
