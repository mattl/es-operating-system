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

#ifndef NINTENDO_ES_HASHTABLE_H_INCLUDED
#define NINTENDO_ES_HASHTABLE_H_INCLUDED

#include <new>
#include <functional>   // equal_to<>
#include <errno.h>
#include <es.h>
#include <es/exception.h>
#include <es/uuid.h>

template <class K>
struct Hash
{
};

template<>
struct Hash<Guid>
{
    size_t operator()(const Guid& guid) const
    {
        return reinterpret_cast<const u32*>(&guid)[0] ^
               reinterpret_cast<const u32*>(&guid)[1] ^
               reinterpret_cast<const u32*>(&guid)[2] ^
               reinterpret_cast<const u32*>(&guid)[3];
    }
};

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
