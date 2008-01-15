/*
 * Copyright 2008 Google Inc.
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
#include <es.h>
#include "arena.h"

void* Arena::
alloc(size_t size, size_t align) throw()
{
    Lock::Synchronized method(spinLock);

    Cell*  cell;
    size_t padding;

    ASSERT(align % ALIGN == 0);
    ASSERT(0 < size && size % ALIGN == 0);

    // Search for a cell large enough
    Cell::List::Iterator iter = unused.begin();
    while ((cell = iter.next()))
    {
        padding = round(cell->left(), align) - cell->left();
        if (padding + size <= cell->size)
        {
            break;
        }
    }
    if (!cell)
    {
        return 0;
    }

    char* place;
    size_t leftover = cell->size - size;
    if (padding == 0)
    {
        place = const_cast<char*>(cell->left());
        iter.remove();
    }
    else
    {
        leftover -= padding;
        cell->size = padding;
        place = const_cast<char*>(cell->right());
    }
    if (0 < leftover)
    {
        cell = new(place + size) Cell(leftover);
        iter.add(cell);
    }

    return static_cast<void*>(place);
}

void* Arena::
allocLast(size_t size, size_t align) throw()
{
    Lock::Synchronized method(spinLock);

    Cell*  cell;
    size_t padding;

    ASSERT(align % ALIGN == 0);
    ASSERT(0 < size && size % ALIGN == 0);

    // Search for a cell large enough
    Cell::List::Iterator iter = unused.end();
    while ((cell = iter.previous()))
    {
        padding = round(cell->left(), align) - cell->left();
        if (padding + size <= cell->size)
        {
            break;
        }
    }
    if (!cell)
    {
        return 0;
    }

    char* place;
    size_t leftover = cell->size - size;
    if (padding == 0)
    {
        place = const_cast<char*>(cell->left());
        iter.remove();
    }
    else
    {
        leftover -= padding;
        cell->size = padding;
        place = const_cast<char*>(cell->right());
        iter.next();
    }
    if (0 < leftover)
    {
        cell = new(place + size) Cell(leftover);
        iter.add(cell);
    }

    return static_cast<void*>(place);
}

void Arena::
free(void* place, size_t size) throw()
{
    Lock::Synchronized method(spinLock);

    ASSERT(reinterpret_cast<size_t>(place) % ALIGN == 0);
    ASSERT(0 < size && size % ALIGN == 0);

    Cell* cell = new(place) Cell(size);
    Cell* next;
    Cell* prev = NULL;

    Cell::List::Iterator iter = unused.begin();
    while ((next = iter.next()))
    {
        if (cell->left() <= next->left())
        {
            iter.previous();
            break;
        }
        prev = next;
    }
    iter.add(cell);

    // Coalesce if possible
    if (next && cell->right() == next->left())
    {
        cell->size += next->size;
        unused.remove(next);
    }
    if (prev && prev->right() == cell->left())
    {
        prev->size += cell->size;
        unused.remove(cell);
    }
}

size_t Arena::
size() throw()
{
    Lock::Synchronized method(spinLock);

    Cell* cell;
    size_t size = 0;

    Cell::List::Iterator iter = unused.begin();
    while ((cell = iter.next()))
    {
        size += cell->size;
    }
    return size;
}
