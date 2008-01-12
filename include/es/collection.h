/*
 * Copyright (c) 2007
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

#ifndef NINTENDO_ES_COLLECTION_H_INCLUDED
#define NINTENDO_ES_COLLECTION_H_INCLUDED

#include <errno.h>
#include <es.h>
#include <es/exception.h>
#include <es/list.h>

struct CollectionLink
{
    Link<CollectionLink>  link;

    typedef List<CollectionLink, &CollectionLink::link> List;
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

    Iterator list(E item)
    {
        return Iterator(nodeList.begin());
    }
};

#endif // NINTENDO_ES_COLLECTION_H_INCLUDED
