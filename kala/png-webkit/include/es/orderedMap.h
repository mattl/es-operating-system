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

#ifndef GOOGLE_ES_ORDEREDMAP_H_INCLUDED
#define GOOGLE_ES_ORDEREDMAP_H_INCLUDED

#include <string>
#include <cstring>
#include <algorithm>
#include <es/ref.h>
#include <es/tree.h>
#include <es/util/IOrderedMap.h>

class OrderedMap : public es::OrderedMap
{
    typedef Tree<std::string, std::string> MapTree;
    MapTree tree;
    int size;
    Ref ref;

public:
    OrderedMap() :
        tree(),
        size(0)
    {}

    ~OrderedMap()
    {
    }

    int getSize()
    {
        return size;
    }

    const char* getByIndex(void* string, int stringLength, unsigned int index)
    {
        int count = 0;
        MapTree::Iterator it = tree.begin();
        while (MapTree::Node* node = it.next())
        {
            if (count == index)
            {
                std::string& str = node->getValue();
                int len = std::min((int)str.length(), stringLength);
                std::memmove(string, str.c_str(), len);
                return static_cast<char*>(string);
            }
            count++;
        }
        return 0;
    }

    void setByIndex(unsigned int index, const char* value)
    {
        if (index > size)
        {
            return;
        }
        int count = 0;
        MapTree::Node* node;
        MapTree::Iterator it = tree.begin();
        while (node = it.next())
        {
            if (count == index)
            {
                std::string key = node->getKey();
                std::string str(value);
                tree.remove(key);
                tree.add(key, str);
                break;
            }
        }
    }

    const char* get(void* string, int stringLength, const char* name)
    {
        std::string key(name);
        try
        {
            std::string str = tree.get(key);
            int len = std::min((int)str.length(), stringLength);
            std::memmove(string, str.c_str(), len);
            return static_cast<char*>(string);
        }
        catch (...)
        {
            return 0;
        }
    }

    void set(const char* name, const char* value)
    {
        std::string key(name);
        std::string str(value);
        if (tree.contains(key))
        {
            tree.remove(key);
            size--;
        }
        tree.add(key, str);
        size++;
    }

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::OrderedMap::iid()) == 0)
        {
            objectPtr = static_cast<es::OrderedMap*>(this);
        }
        else if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::OrderedMap*>(this);
        }
        else
        {
            return NULL;
        }
        objectPtr->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        int count = ref.addRef();
        return count;
    }

    unsigned int release()
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

#endif  // GOOGLE_ES_ORDEREDMAP_H_INCLUDED
