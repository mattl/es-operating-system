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

#ifndef NINTENDO_ES_HASHTABLE_H_INCLUDED
#define NINTENDO_ES_HASHTABLE_H_INCLUDED

#include <new>
#include <functional>   // equal_to<>
#include <errno.h>
#include <stdint.h>
#include <es.h>
#include <es/exception.h>

template <class K>
struct Hash
{
};

template<>
struct Hash<const char*>
{
    size_t operator()(const char* name) const
    {
        size_t hash = 0;
        while (*name)
        {
            hash = (hash << 1) ^ *name++;
        }
        return hash;
    }
};

// usage: Hashtable<Guid, Reflect::Interface> hashtable;
template <class K, class V,
          class H = Hash<K>,
          class EQ = std::equal_to<K> >
class Hashtable
{
    struct Element
    {
        K        key;
        V        value;
        Element* next;
    };

    Element*  entries;  // the actual entries
    Element** table;    // the hash table
    Element*  free;

    int       capacity; // size
    H         hash;     // hash function
    EQ        eq;       // equality

    Element* allocElement()
    {
        if (!free)
        {
            throw SystemException<ENOSPC>();
        }
        Element* elm = free;
        free = elm->next;
        return elm;
    }

    void freeElement(Element* elm)
    {
        elm->next = free;
        free = elm;
    }

public:
    Hashtable(int capacity, const H& hash = H(), const EQ& eq = EQ()) :
        free(0), capacity(capacity), hash(hash), eq(eq)
    {
        ASSERT(0 < capacity);
        entries = new Element[size()];
        table = new Element*[size()];
        for (int i = 0; i < size(); ++i)
        {
            Element* elm = &entries[i];
            elm->next = free;
            free = elm;

            table[i] = 0;
        }
    }

    ~Hashtable()
    {
        delete[] table;
        delete[] entries;
    }

    int size() const
    {
        ASSERT(0 < capacity);
        return capacity;
    }

    void clear()
    {
        for (int i = 0; i < size(); ++i)
        {
            while (Element* elm = table[i])
            {
                table[i] = elm->next;
                freeElement(elm);
            }
        }
    }

    bool contains(const K& key) const
    {
        size_t i = hash(key) % size();
        for (Element* elm = table[i]; elm; elm = elm->next)
        {
            if (eq(key, elm->key))
            {
                return true;
            }
        }
        return false;
    }

    V& get(const K& key) const
    {
        size_t i = hash(key) % size();
        for (Element* elm = table[i]; elm; elm = elm->next)
        {
            if (eq(key, elm->key))
            {
                return elm->value;
            }
        }

        // Not found:
        throw SystemException<ENOENT>();
    }

    void add(const K& key, V value)
    {
        size_t i = hash(key) % size();
        for (Element* elm = table[i]; elm; elm = elm->next)
        {
            if (eq(key, elm->key))
            {
                // An element already exists:
                throw SystemException<EEXIST>();
            }
        }

        // Not found:
        Element* elm = allocElement();
        elm->key = key;
        elm->value = value;
        elm->next = table[i];
        table[i] = elm;
    }

    bool remove(const K& key)
    {
        size_t i = hash(key) % size();
        Element** prev;
        Element* elm;
        for (prev = &table[i], elm = table[i];
             elm;
             prev = &elm->next, elm = elm->next)
        {
            if (eq(key, elm->key))
            {
                *prev = elm->next;
                freeElement(elm);
                return true;
            }
        }
        return false;
    }
};

#endif // NINTENDO_ES_HASHTABLE_H_INCLUDED
