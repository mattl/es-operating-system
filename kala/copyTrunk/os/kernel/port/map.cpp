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

#include <new>
#include <string.h>
#include <stdlib.h>
#include <es.h>
#include "core.h"
#include "process.h"

Map::
Map(Process* proc, const void* start, const void* end, long length,
    unsigned prot, unsigned flags,
    es::Pageable* pageable, long long offset) :
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
