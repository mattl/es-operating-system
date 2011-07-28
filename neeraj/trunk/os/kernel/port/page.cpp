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

#include <string.h>
#include <new>
#include "cache.h"

Page::
Page(void* pointer) :
    ref(0),
    cache(0),
    offset(0),
    flags(0),
    pointer(pointer),
    pageSet(0),
    map(0),
    filled(false),
    lastUpdated(0)
{
}

void Page::
set(Cache* cache, long long offset)
{
    cache->incPageCount();
    this->cache = cache;
    this->offset = offset;
    this->flags = 0;
    this->filled = false;
    PageTable::add(this);
}

int Page::
fill(es::Stream* backingStore)
{
    int len = SIZE;

    if (!filled)
    {
        Monitor::Synchronized method(monitor);

        while (!filled)
        {
            u8* ptr = static_cast<u8*>(getPointer());
            int n;
            for (len = 0; len < SIZE; len += n, ptr += n)
            {
                n = backingStore->read(ptr, SIZE - len, offset + len);
                if (n <= 0)
                {
                    memset(ptr, 0, SIZE - len);
                    break;
                }
            }
            filled = true;
        }
    }
    return len;
}

int Page::
sync(es::Stream* backingStore, int sectorSize)
{
    if (!cache->clean(this))
    {
        return Page::SIZE;
    }

    // Turn off 'map'.
    u64 map;
    {
        SpinLock::Synchronized method(spinLock);
        map = this->map;
        this->map = 0;
    }

    Monitor::Synchronized method(monitor);

    u8* ptr = static_cast<u8*>(getPointer());
    int from = 0;
    int len;
    int shift = sectorSize / SECTOR;
    u64 mask = (1LLu << shift) - 1;
    while (map)
    {
        if (!(map & mask))
        {
            from += sectorSize;
            mask <<= shift;
            continue;
        }

        len = 0;
        do {
            len += sectorSize;
            map &= ~mask;
            mask <<= shift;
        } while (map & mask);

        // Write back 'from' to 'from + len'.
        while (0 < len)
        {
            int n = backingStore->write(ptr + from, len, offset + from);
            if (n <= 0)
            {
                from += n;
                break;
            }
            from += n;
            len -= n;
        }
    }
    return from;
}

void Page::
touch()
{
    DateTime now(DateTime::getNow());
    if (lastUpdated < now)
    {
        lastUpdated = now;
    }
    else
    {
        // For the low resolution timer
        lastUpdated = lastUpdated.addTicks(1);
    }
}

bool Page::
isStale()
{
    return (Cache::DelayedWrite < DateTime::getNow().getTicks() - lastUpdated.getTicks()) ? true : false;
}

void Page::
free()
{
    {
        SpinLock::Synchronized method(spinLock);
        flags |= Free;
    }

    PageTable::remove(this);

    cache->clean(this);
    release();
}

void Page::
change()
{
    cache->change(this);
    release();
}

int Page::
read(void* dst, int count, long long offset)
{
    ASSERT(0 <= count && count <= SIZE);
    ASSERT(0 <= offset && offset < SIZE);
    memmove(dst, static_cast<u8*>(pointer) + offset, count);
    return count;
}

int Page::
write(const void* src, int count, long long offset)
{
    ASSERT(0 <= count && count <= SIZE);
    ASSERT(0 <= offset && offset < SIZE);
    memmove(static_cast<u8*>(pointer) + offset, src, count);

    int bits = (offset + count + SECTOR - 1) / SECTOR;
    offset /= SECTOR;
    bits -= offset;
    {
        SpinLock::Synchronized method(spinLock);
        map |= (((1u << bits) - 1) << offset);
    }
    return count;
}

unsigned int Page::
addRef(void)
{
    return ref.addRef();
}

unsigned int Page::
release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        ASSERT(!(flags & Changed));
        if (flags & Free)
        {
            pageSet->free(this);
        }
        else
        {
            pageSet->standby(this);
        }
    }
    return count;
}
