/*
 * Copyright 2008, 2009 Google Inc.
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

#ifndef GOOGLE_ES_OBJECT_H_INCLUDED
#define GOOGLE_ES_OBJECT_H_INCLUDED

#include <new>
#include <functional>   // equal_to<>
#include <errno.h>
#include <es.h>
#include <es/interlocked.h>
#include <es/synchronized.h>
#include <es/base/IMonitor.h>

// usage: ObjectTable<Capability, Imported, 1024> exportedTable;
//
// K and V must have the following method
//    size_t hash()
// and
//    V::isMatch(const K& k)
template <class K, class V, int capacity>
class ObjectTable
{
    class Ref
    {
        Interlocked ref;

    public:
        /**
        * Constructs a reference count.
        */
        Ref() : ref(0)
        {
        }

        /**
        * A conversion operator.
        */
        operator long() const
        {
            return ref;
        }

        /**
        * Sets this reference count to one.
        */
        void init()
        {
            long count = ref.exchange(1);
            ASSERT(count == 0);
        }

        /**
        * Increments this reference count.
        * @return the new reference count.
        */
        long addRef()
        {
            long count;
            do {
                count = ref;
                if (count == 0)     // TODO - Also check the upper bound
                {
                    return 0;
                }
            } while (ref.compareExchange(count + 1, count) != count);
            return count + 1;
        }

        /**
        * Decrements this reference count.
        * @return the new reference count.
        */
        long release()
        {
            long count;
            do {
                count = ref;
                if (count <= 0)
                {
                    return -1;
                }
            } while (ref.compareExchange(count - 1, count) != count);
            return count - 1;
        }
    };

    struct Element
    {
        int      index;
        V        value;
        Element* next;

        Element(int index, const K& key) :
            index(index),
            value(key),
            next(0)
        {
        }
    };

es::Monitor*        monitor;
    int             free;                 // free descriptor number (-1 if none)
    Ref             refs[capacity];
    Element*        entries[capacity];    // the actual entries (LSB = 1 if free)
    Element*        table[capacity];      // the hash table

    bool isFree(int index)
    {
        return (*reinterpret_cast<long*>(&entries[index]) & 1) ? true : false;
    }

    int allocEntry()
    {
        int index = free;
        if (0 <= index)
        {
            free = *reinterpret_cast<long*>(&entries[index]) >> 1;
        }
        return index;
    }

    void freeEntry(int index)
    {
        *reinterpret_cast<long*>(&entries[index]) = (free << 1) | 1;
        free = index;
        ASSERT(isFree(free));
    }

    Element* remove(int index)
    {
        Synchronized<es::Monitor*> method(monitor);

        ASSERT(0 <= index && index < size());
        ASSERT(!isFree(index));

        Element* elm = entries[index];
        freeEntry(index);

        size_t i = elm->value.hash() % size();
        Element* cur;
        Element** prev;
        for (prev = &table[i], cur = table[i];
             cur;
             prev = &cur->next, cur = cur->next)
        {
            if (cur == elm)
            {
                *prev = elm->next;
                return elm;
            }
        }
        return 0;
    }

public:
    ObjectTable(es::Monitor* monitor) :
        monitor(monitor),
        free(-1)
    {
        ASSERT(monitor);
        ASSERT(((-1 << 1) | 1) == -1);
        ASSERT((-1 >> 1) == -1);
        for (int i = size() - 1; 0 <= i; --i)
        {
            table[i] = 0;
            freeEntry(i);
        }
    }

    ~ObjectTable()
    {
        monitor->release();
    }

    int size() const
    {
        return capacity;
    }

    // If key has been added already, the reference count for that entry is incremented by one.
    int add(const K& key)
    {
        Synchronized<es::Monitor*> method(monitor);

        size_t i = key.hash() % size();
        for (Element* elm = table[i]; elm; elm = elm->next)
        {
            if (elm->value.isMatch(key))
            {
                // Found
                int index = elm->index;
                if (0 < refs[index].addRef())
                {
                    return index;
                }
                else
                {
                    break;  // elm is being deleted.
                }
            }
        }

        // Not found. Create a new entry

        int index = allocEntry();
        if (index < 0)
        {
            return -1;
        }

        Element* elm = new(std::nothrow) Element(index, key);
        if (!elm)
        {
            freeEntry(index);
            return -1;
        }

        elm->next = table[i];
        table[i] = elm;

        entries[index] = elm;
        ASSERT(!isFree(index));

        refs[index].init();    // Set to 1 again

        return index;
    }

    V* get(int index)
    {
        if (index < 0 || size() <= index)
        {
            return 0;
        }
        long count = refs[index].addRef();
        if (count == 0)
        {
            return 0;
        }
        Element* elm = entries[index];
        ASSERT((reinterpret_cast<long>(elm) & 1) == 0);
        return &elm->value;
    }

    int put(int index)
    {
        if (index < 0 || size() <= index)
        {
            return 0;
        }
        int count = refs[index].release();
        if (count == 0)
        {
            Element* elm = remove(index);
            ASSERT(elm);

            delete elm;
        }
        return count;
    }

    void lock()
    {
        monitor->lock();
    }

    void unlock()
    {
        monitor->unlock();
    }
};

#endif // GOOGLE_ES_OBJECT_H_INCLUDED
