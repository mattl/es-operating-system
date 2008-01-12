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

#include <new>
#include <string.h>
#include <stdlib.h>
#include <es.h>
#include "core.h"
#include "process.h"

Map::
Map(Process* proc, const void* start, const void* end, long length,
    unsigned prot, unsigned flags,
    IPageable* pageable, long long offset) :
        proc(proc),
        start(start),
        end(Page::round(end)),
        length(length),
        prot(prot),
        flags(flags),
        pageable(pageable),
        offset(offset)
{
#ifdef VERBOSE
    esReport("Map(%p, %p, %p) %p\n", start, end, length, Page::round(end));
#endif  // VERBOSE
    ASSERT(Page::trunc(start) == start);
    ASSERT(Page::trunc(this->end) == this->end);
    ASSERT(Page::pageOffset(offset) == 0);
    ASSERT(0 <= length && length <= (u8*) end - (u8*) start);
    if (pageable)
    {
        pageable->addRef();
    }
}

Map::
~Map()
{
    if (pageable)
    {
        pageable->release();
    }
}

long long Map::
getPosition(const void* addr)
{
    ASSERT(start <= addr && addr < end);
    return offset + (static_cast<const u8*>(addr) - static_cast<const u8*>(start));
}
