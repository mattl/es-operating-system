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
#include <es.h>
#include "arena.h"

void* Arena::
alloc(size_t size, size_t align) throw()
{
    SpinLock::Synchronized method(spinLock);

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
    SpinLock::Synchronized method(spinLock);

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
    SpinLock::Synchronized method(spinLock);

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
    SpinLock::Synchronized method(spinLock);

    Cell* cell;
    size_t size = 0;

    Cell::List::Iterator iter = unused.begin();
    while ((cell = iter.next()))
    {
        size += cell->size;
    }
    return size;
}
