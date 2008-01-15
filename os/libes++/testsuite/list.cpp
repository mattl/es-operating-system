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
