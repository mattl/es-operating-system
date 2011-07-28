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
#include <string.h>
#include <stdlib.h>
#include <es.h>
#include "heap.h"

const size_t Heap::Mass::SIZE = (sizeof(Mass) + Arena::ALIGN - 1) & ~(Arena::ALIGN - 1);
const size_t Heap::Cell::SIZE = (sizeof(Cell) + Arena::ALIGN - 1) & ~(Arena::ALIGN - 1);

Heap::
Heap(Arena& arena) : arena(arena)
{
    for (size_t i = 0; i < BUCKET_SIZE; ++i)
    {
        Bucket& bucket = buckets[i];
        bucket.arena = &arena;

        size_t d = (Page::SIZE - Mass::SIZE) / ((Arena::ALIGN << i) + Cell::SIZE);
        if (2 < d)
        {
            bucket.size = (((Page::SIZE - Mass::SIZE) / d) & ~(Arena::ALIGN - 1)) - Cell::SIZE;
        }
        else
        {
            d = 2;
            bucket.size = (((Page::SIZE - Mass::SIZE) / d) & ~(Arena::ALIGN - 1)) - Cell::SIZE;
            thresh = bucket.size;
            break;
        }
    }
}

Heap::Bucket* Heap::
getBucket(size_t size)
{
    ASSERT(size <= thresh);
    int i = ffs(size);
    if (i == 0)
    {
        return 0;
    };
    if (i <= Arena::SHIFT)
    {
        i = 0;
    }
    else
    {
        i -= Arena::SHIFT + 1;
    }
    Bucket* bucket;
    for (bucket = &buckets[i]; bucket->size < size; ++bucket)
    {
    }
    return bucket;
}

Heap::
~Heap()
{
    for (size_t i = 0; i < BUCKET_SIZE; ++i)
    {
        Bucket& bucket = buckets[i];
        while (!bucket.listMass.isEmpty())
        {
            Mass* mass = bucket.listMass.removeFirst();
            bucket.freeMass(mass);
        }
    }

    while (!listLargeCell.isEmpty())
    {
        Cell* cell = listLargeCell.removeFirst();
        arena.free(cell, cell->size);
    }
}

void* Heap::
alloc(size_t size)
{
    if (isLargeCell(size))
    {
        size = (size + Arena::ALIGN - 1) & ~(Arena::ALIGN - 1);
        void* place = arena.alloc(size + Cell::SIZE, Arena::ALIGN);
        if (place)
        {
            Lock::Synchronized method(spinLock);

            Cell* cell = new(place) Cell(size);
            listLargeCell.addLast(cell);
            return cell->getData();
        }
    }
    else
    {
        Bucket* bucket = getBucket(size);
        ASSERT(bucket);
        Cell* cell = bucket->alloc(size);
        if (cell)
        {
            return cell->getData();
        }
    }
    return 0;
}

void Heap::
free(void* place)
{
    if (place)
    {
        Cell* cell = getCell(place);
        if (isLargeCell(cell->size))
        {
            Lock::Synchronized method(spinLock);

            listLargeCell.remove(cell);
            arena.free(cell, cell->size + Cell::SIZE);
        }
        else
        {
            Mass* mass = getMass(cell);
            mass->bucket->free(mass, cell);
        }
    }
}

void* Heap::
realloc(void *ptr, size_t size)
{
    if (size == 0)
    {
        free(ptr);
        return 0;
    }

    void* resized = malloc(size);
    if (resized && ptr)
    {
        Cell* from = getCell(ptr);
        Cell* to = getCell(resized);
        ASSERT(from && to);
        memmove(resized, ptr, (from->size < to->size) ? from->size : to->size);
        free(ptr);
    }
    return resized;
}

Heap::
Mass::Mass(Bucket* bucket) : used(0), bucket(bucket)
{
    ASSERT(Cell::SIZE <= bucket->size);
    char* left = reinterpret_cast<char*>(this);
    char* right = left + Page::SIZE;
    for (char *place = left + SIZE, *next = place + Cell::SIZE + bucket->size;
         next <= right;
         place = next, next += Cell::SIZE + bucket->size)
    {
        Cell* cell = new(place) Cell(bucket->size);
        listCell.addLast(cell);
    }
}

Heap::Cell* Heap::
Mass::getCell()
{
    Cell* cell = listCell.removeFirst();
    if (cell)
    {
        ++used;
    }
    return cell;
}

size_t Heap::
Mass::putCell(Cell* cell)
{
    listCell.addFirst(cell);
    return --used;
}

Heap::Cell* Heap::
Bucket::alloc(size_t size)
{
    Lock::Synchronized method(spinLock);

    Mass* mass;
    Mass::List::Iterator iter = listMass.begin();
    while ((mass = iter.next()))
    {
        Cell* cell = mass->getCell();
        if (cell)
        {
            return cell;
        }
    }

    // Allocate new mass
    void* place = allocMass();
    if (!place)
    {
        return 0;
    }
    mass = new(place) Mass(this);
    listMass.addFirst(mass);
    return mass->getCell();
}

size_t Heap::
Bucket::free(Mass* mass, Cell* cell)
{
    Lock::Synchronized method(spinLock);

    if (mass->putCell(cell) == 0)
    {
        listMass.remove(mass);
        freeMass(mass);
    }
}
