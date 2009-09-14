/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_COLLECTION_H_INCLUDED
#define NINTENDO_ES_COLLECTION_H_INCLUDED

#include <errno.h>
#include <es.h>
#include <es/exception.h>
#include <es/list.h>

struct CollectionLink
{
    Link<CollectionLink>  link;

    typedef ::List<CollectionLink, &CollectionLink::link> List;
};

template <class E>
class Collection
{
    class Node : public CollectionLink
    {
        friend class Collection;
        friend class Collection::Iterator;

        E   elm;

    public:
        Node(E elm) : elm(elm) {}
    };

    CollectionLink::List    nodeList;

public:

    class Iterator
    {
        friend class Collection;

        CollectionLink::List::Iterator  iter;

        Iterator(CollectionLink::List::Iterator iter) : iter(iter) {}

    public:
        bool hasNext() const
        {
            return iter.hasNext();
        }

        E next()
        {
            if (Node* node = static_cast<Node*>(iter.next()))
            {
                return node->elm;
            }
            throw SystemException<ENOENT>();
        }

        bool hasPrevious() const
        {
            return iter.hasPrevious();
        }

        E previous()
        {
            if (Node* node = static_cast<Node*>(iter.previous()))
            {
                return node->elm;
            }
            throw SystemException<ENOENT>();
        }

        E remove()
        {
            if (Node* node = static_cast<Node*>(iter.remove()))
            {
                E item = node->elm;
                delete node;
                return item;
            }
            throw SystemException<ENOENT>();
        }

        void add(E item)
        {
            Node* node = new Node(item);
            iter.add(node);
        }
    };

    E getFirst() const
    {
        if (Node* node = static_cast<Node*>(nodeList.getFirst()))
        {
            return node->elm;
        }
        throw SystemException<ENOENT>();
    }

    E getLast() const
    {
        if (Node* node = static_cast<Node*>((nodeList.getLast())))
        {
            return node->elm;
        }
        throw SystemException<ENOENT>();
    }

    bool isEmpty() const
    {
        return nodeList.isEmpty();
    }

    bool addAll(Collection* collection)
    {
        return nodeList.addAll(collection->nodeList);
    }

    void addFirst(E item)
    {
        Node* node = new Node(item);
        nodeList.addFirst(node);
    }

    void addLast(E item)
    {
        Node* node = new Node(item);
        nodeList.addLast(node);
    }

    E remove(E item)
    {
        CollectionLink::List::Iterator  iter(nodeList.begin());
        while (Node* node = static_cast<Node*>(iter.next()))
        {
            if (node->elm == item)
            {
                iter.remove();
                delete node;
                return item;
            }
        }
        throw SystemException<ENOENT>();
    }

    E removeFirst()
    {
        if (Node* node = static_cast<Node*>(nodeList.removeFirst()))
        {
            E item = node->elm;
            delete node;
            return item;
        }
        throw SystemException<ENOENT>();
    }

    E removeLast()
    {
        if (Node* node = static_cast<Node*>(nodeList.removeLast()))
        {
            E item = node->elm;
            delete node;
            return item;
        }
        throw SystemException<ENOENT>();
    }

    Iterator begin()
    {
        return Iterator(nodeList.begin());
    }

    Iterator end()
    {
        return Iterator(nodeList.end());
    }

    bool contains(E item)
    {
        CollectionLink::List::Iterator  iter(nodeList.begin());
        while (Node* node = static_cast<Node*>(iter.next()))
        {
            if (node->elm == item)
            {
                return true;
            }
        }
        return false;
    }
};

#endif // NINTENDO_ES_COLLECTION_H_INCLUDED
