/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_TREE_H_INCLUDED
#define NINTENDO_ES_TREE_H_INCLUDED

#include <errno.h>
#include <new>
#include <functional>   // less<>
#include <es.h>
#include <es/exception.h>

// Arne Andersson Tree template class
//
// cf. A. Andersson. Balanced search trees made simple.
//     In Proc. Workshop on Algorithms and Data Structures,
//     pages 60-71. Springer Verlag, 1993.
//     <http://user.it.uu.se/~arnea/abs/simp.html>

template <class K, class V,
          class LE = std::less<K> >
class Tree
{
public:
    class Node
    {
        friend class Tree;

        static Node*    nil;

        K       key;
        V       value;
        int     level;
        Node*   left;
        Node*   right;
        Node*   parent;

        Node() :
            level(0),
            left(nil),
            right(nil),
            parent(nil)
        {
        }

        Node(K key, V value) :
            key(key),
            value(value),
            level(1),
            left(nil),
            right(nil),
            parent(nil)
        {
        }

        // Rotate right
        Node* skew()
        {
            Node* root = this;
            if (level != 0)
            {
                if (left->level == level)
                {
                    root = left;
                    root->parent = nil;
                    left = left->right;
                    left->parent = this;
                    root->right = this;
                    parent = root;
                }
                root->right = root->right->skew();
            }
            return root;
        }

        // Rotate left
        Node* split()
        {
            Node* root = this;
            if (level != 0 && right->right->level == level)
            {
                root = right;
                root->parent = nil;
                right = root->left;
                right->parent = this;
                root->left = this;
                parent = root;
                ++root->level;
                root->right = root->right->split();
            }
            return root;
        }

        Node* insert(K key, V value, const LE& le)
        {
            Node* root = this;
            if (this == nil)
            {
                root = new Node(key, value);
            }
            else
            {
                if (le(this->key, key))
                {
                    right = right->insert(key, value, le);
                    right->parent = this;
                }
                else if (le(key, this->key))
                {
                    left = left->insert(key, value, le);
                    left->parent = this;
                }
                else
                {
                    // Already exists:
                    throw SystemException<EEXIST>();
                }
                root = skew()->split();
            }
            return root;
        }

        Node* remove(K key, const LE& le)
        {
            Node* root = this;
            if (this != nil)
            {
                if (le(this->key, key))
                {
                    right = right->remove(key, le);
                    right->parent = this;
                }
                else if (le(key, this->key))
                {
                    left = left->remove(key, le);
                    left->parent = this;
                }
                else
                {
                    if (left == nil)
                    {
                        root = right;
                        root->parent = nil;
                        delete this;
                    }
                    else if (right == nil)
                    {
                        root = left;
                        root->parent = nil;
                        delete this;
                    }
                    else
                    {
                        Node* heir = left;
                        while (heir->right != nil)
                        {
                            heir = heir->right;
                        }
                        this->key = heir->key;
                        this->value = heir->value;
                        left = left->remove(this->key, le);
                        left->parent = this;
                    }
                }
            }

            if (root->left->level < root->level - 1 ||
                root->right->level < root->level - 1)
            {
                if (--root->level < root->right->level)
                {
                    root->right->level = root->level;
                }
                root = root->skew()->split();
            }
            return root;
        }

    public:
        int getLevel() const
        {
            return level;
        }

        Node* getFirst() const
        {
            const Node* node = this;
            while (node->left != nil)
            {
                node = node->left;
            }
            return const_cast<Node*>(node);
        }

        Node* getLast() const
        {
            const Node* node = this;
            while (node->right != nil)
            {
                node = node->right;
            }
            return const_cast<Node*>(node);
        }

        Node* getNext() const
        {
            const Node* node = this;
            if (node->right != nil)
            {
                return node->right->getFirst();
            }

            const Node* last;
            do
            {
                if (node->parent == nil)
                {
                    return nil;
                }
                last = node;
                node = node->parent;
            } while (last == node->right);
            return const_cast<Node*>(node);
        }

        Node* getPrevious() const
        {
            const Node* node = this;
            if (node->left != nil)
            {
                return node->left->getLast();
            }

            const Node* last;
            do
            {
                if (node->parent == nil)
                {
                    return nil;
                }
                last = node;
                node = node->parent;
            } while (last == node->left);
            return const_cast<Node*>(node);
        }

        Node* getParent() const
        {
            return (parent != nil) ? parent : 0;
        }

        V& getValue()
        {
            return value;
        }

        K& getKey()
        {
            return key;
        }
    };

    class Iterator
    {
        static Node*    nil;

        Node*   pre;
        Node*   suc;
        Node*   cur;

    public:
        Iterator(Node* pre, Node* suc) throw() :
            pre(pre),
            suc(suc),
            cur(0)
        {
        }

        bool hasNext() const
        {
            return (suc == nil) ? true : false;
        }

        Node* next()
        {
            cur = suc;
            if (suc != nil)
            {
                pre = suc;
                suc = suc->getNext();
            }
            return (cur != nil) ? cur : 0;
        }

        bool hasPrevious() const
        {
            return (pre == nil) ? true : false;
        }

        Node* previous()
        {
            cur = pre;
            if (pre != nil)
            {
                suc = pre;
                pre = pre->getPrevious();
            }
            return (cur != nil) ? cur : 0;
        }
    };

private:
    static Node     nil;

    LE              le;         // less than
    Node*           root;

public:
    Tree(const LE& le = LE()) :
        le(le),
        root(&nil)
    {
    }

    ~Tree()
    {
    }

    bool isEmpty() const
    {
        return (root == &nil) ? true : false;
    }

    Node* getFirst() const
    {
        Node* node = root->getFirst();
        return (node != &nil) ? node : 0;
    }

    Node* getLast() const
    {
        Node* node = root->getLast();
        return (node != &nil) ? node : 0;
    }

    bool contains(const K& key) const
    {
        Node* node = root;
        while (node != &nil)
        {
            if (le(node->key, key))
            {
                node = node->right;
            }
            else if (le(key, node->key))
            {
                node = node->left;
            }
            else
            {
                return true;
            }
        }
        return false;
    }

    V& get(const K& key) const
    {
        Node* node = root;
        while (node != &nil)
        {
            if (le(node->key, key))
            {
                node = node->right;
            }
            else if (le(key, node->key))
            {
                node = node->left;
            }
            else
            {
                return node->value;
            }
        }

        // Not found:
        throw SystemException<ENOENT>();
    }

    void add(const K& key, V value)
    {
        root = root->insert(key, value, le);
    }

    bool remove(const K& key)
    {
        root = root->remove(key, le);
    }

    Iterator begin()
    {
        return Iterator(&nil, root->getFirst());
    }

    Iterator end()
    {
        return Iterator(root->getLast(), &nil);
    }
};

template <class K, class V, class LE>
    typename Tree<K, V, LE>::Node Tree<K, V, LE>::nil;

template <class K, class V, class LE>
    typename Tree<K, V, LE>::Node* Tree<K, V, LE>::Node::nil = &Tree<K, V, LE>::nil;

template <class K, class V, class LE>
    typename Tree<K, V, LE>::Node* Tree<K, V, LE>::Iterator::nil = &Tree<K, V, LE>::nil;

#endif // NINTENDO_ES_TREE_H_INCLUDED
