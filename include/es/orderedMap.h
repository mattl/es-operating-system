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

using namespace es;

class OrderedMap : public IOrderedMap
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

    int getByIndex(char* string, int stringLength, int index)
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
                return len;
            }
            count++;
        }
        return 0;
    }

    void setByIndex(int index, const char* value)
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

    int get(char* string, int stringLength, const char* name)
    {
        std::string key(name);
        try
        {
            std::string str = tree.get(key);
            int len = std::min((int)str.length(), stringLength);
            std::memmove(string, str.c_str(), len);
            return len;
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

    void* queryInterface(const char* riid)
    {
        void* objectPtr;
        if (strcmp(riid, IOrderedMap::iid()) == 0)
        {
            objectPtr = static_cast<IOrderedMap*>(this);
        }
        else if (strcmp(riid, IInterface::iid()) == 0)
        {
            objectPtr = static_cast<IOrderedMap*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
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
