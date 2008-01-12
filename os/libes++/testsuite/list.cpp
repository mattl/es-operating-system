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

#include <new>
#include <iostream>
#include <es/list.h>

class Node
{
public:
    Node(int n) : val(n) { };
    int Val() { return val; };

    int         val;
    Link<Node>  link;
};

int main()
{
    Node* t;
    int a[10];
    int i;

    List<Node, &Node::link> list;
    for (i = 0; i < 5; ++i)
    {
        t = new Node(i);
        list.addLast(t);
    }

    List<Node, &Node::link>::Iterator iter = list.begin();
    while ((t = iter.next()))
    {
        std::cout << t->Val() << '\n';
        if (t->Val() == 3)
        {
            t = iter.remove();
            delete t;
        }
    }
    while ((t = iter.previous()))
    {
        std::cout << t->Val() << '\n';
        t = iter.remove();
        delete t;
    }
    if (list.isEmpty())
    {
        std::cout << "ok\n";
    }

    t = new Node(100);
    iter.add(t);
    t = new Node(101);
    iter.add(t);
    while ((t = iter.next()))
    {
        std::cout << t->Val() << '\n';
    }
    t = new Node(103);
    iter.add(t);
    while ((t = iter.previous()))
    {
        std::cout << t->Val() << '\n';
        if (t->Val() == 103)
        {
            t = new Node(102);
            iter.add(t);
        }
    }
    t = new Node(99);
    iter.add(t);

    i = 0;
    while ((t = iter.next()))
    {
        a[i++] = t->Val();
    }

    while ((t = iter.previous()))
    {
        std::cout << t->Val() << '\n';
        t = iter.remove();
        delete t;
    }
    if (list.isEmpty())
    {
        std::cout << "ok\n";
    }

    return 0;
}
