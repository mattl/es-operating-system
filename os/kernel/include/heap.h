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

#ifndef NINTENDO_ES_KERNEL_HEAP_H_INCLUDED
#define NINTENDO_ES_KERNEL_HEAP_H_INCLUDED

//
//   buckets[0]       Mass              Cell           Cell           Cell
//  +--------+       +--------+        +-+-+-+----+   +-+-+-+----+   +-+-+-+----+
//  |  head  |------>|  head  |------->|p|n|s|    |---|p|n|s|    |---|p|n|s|    |<-+
//  +--------+       +--------+        |r|e|i|    |   |r|e|i|    |   |r|e|i|    |  |
//  |  tail  |--+    |  tail  |---+    |e|x|z|    |   |e|x|z|    |   |e|x|z|    |  |
//  +--------+  |    +--------+   |    |v|t|e|    |   |v|t|e|    |   |v|t|e|    |  |
//  |  size  |<-|-+  |  used  |   |    +-+-+-+----+   +-+-+-+----+   +-+-+-+----+  |
//  +--------+  | |  +--------+   |                                                |
//              | +--|  bucket|   +------------------------------------------------+
//   buckets[1] |    +--------+
//  +--------+  |    |  prev  |--> 0
//  |  head  |  |    +--------+
//  +--------+  | +->|  next  |---+
//  |  tail  |  | |  +--------+   |
//  +--------+  | |               |
//  |  size  |  | |         +-----+
//  +--------+  | |         |
//              | |   Mass  V           Cell           Cell           Cell
//      :       |    +--------+        +-+-+-+----+   +-+-+-+----+   +-+-+-+----+
//      :       +--->|  head  |------->|p|n|s|    |---|p|n|s|    |---|p|n|s|    |<-+
//      :            +--------+        |r|e|i|    |   |r|e|i|    |   |r|e|i|    |  |
//                |  |  tail  |---+    |e|x|z|    |   |e|x|z|    |   |e|x|z|    |  |
//                |  +--------+   |    |v|t|e|    |   |v|t|e|    |   |v|t|e|    |  |
//                |  |  used  |   |    +-+-+-+----+   +-+-+-+----+   +-+-+-+----+  |
//                |  +--------+   |                                                |
//                |  |  bucket|   +------------------------------------------------+
//                |  +--------+
//                +--|  prev  |
//                   +--------+
//                   |  next  |--> 0
//                   +--------+
//
//   listLargeCell     Cell                    Cell
//  +--------+        +-+-+-+-------------+   +-+-+-+-------------+
//  |  head  |------->|p|n|s|             |---|p|n|s|             |<-+
//  +--------+        |r|e|i|             |   |r|e|i|             |  |
//  |  tail  |---+    |e|x|z|             |   |e|x|z|             |  |
//  +--------+   |    |v|t|e|             |   |v|t|e|             |  |
//               |    +-+-+-+-------------+   +-+-+-+-------------+  |
//               |                                                   |
//               +---------------------------------------------------+
//

#include <cstddef>
#include "arena.h"
#include "cache.h"
#include "thread.h"

class Heap
{
    Lock    spinLock;
    Arena&  arena;
    size_t  thresh;

public:
    Heap(Arena& arena);
    ~Heap();

    void* alloc(size_t size);
    void free(void* place);
    void* realloc(void *ptr, size_t size);

private:
    static const size_t BUCKET_SIZE = 9;

    struct Bucket;

    struct Cell
    {
        static const size_t SIZE;

        size_t      size;       // Cell size
        Link<Cell>  linkCell;

        typedef ::List<Cell, &Cell::linkCell> List;

        explicit Cell(size_t size) : size(size)
        {
        }

        void* getData()
        {
            return reinterpret_cast<char*>(this) + SIZE;
        }
    };

    struct Mass
    {
        static const size_t SIZE;

        int         used;       // Number of cells in use.
        Link<Mass>  linkMass;
        Bucket*     bucket;
        Cell::List  listCell;

        typedef ::List<Mass, &Mass::linkMass> List;

        explicit Mass(Bucket* bucket);
        Cell* getCell();
        size_t putCell(Cell* cell);
    };

    struct Bucket
    {
        Lock        spinLock;
        size_t      size;       // Cell size
        Arena*      arena;
        Mass::List  listMass;

        Bucket() : size(0), arena(0)
        {
        }

        void* allocMass()
        {
            return arena->allocLast(Page::SIZE, Page::SIZE);
        }

        void freeMass(void* place)
        {
            arena->free(place, Page::SIZE);
        }

        Cell* alloc(size_t size);
        size_t free(Mass* mass, Cell* cell);
    };

    // For 28, 60, 124, 252, 504, 1008, 2016 byte objects
    Bucket        buckets[BUCKET_SIZE];
    Cell::List    listLargeCell;

    bool isLargeCell(size_t size)
    {
        return (thresh < size) ? true : false;
    }

    Cell* getCell(void* place)
    {
        return reinterpret_cast<Cell*>(reinterpret_cast<char*>(place) - Cell::SIZE);
    }

    Mass* getMass(const Cell* cell)
    {
        ASSERT(!isLargeCell(cell->size));
        Mass* mass = reinterpret_cast<Mass*>(reinterpret_cast<size_t>(cell) & ~(Page::SIZE - 1));
        ASSERT(mass->bucket->arena == &arena);
        ASSERT(mass->bucket->size == cell->size);
        return mass;
    }

    Bucket* getBucket(size_t size);
};

#endif // NINTENDO_ES_KERNEL_HEAP_H_INCLUDED
