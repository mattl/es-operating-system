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

#include <assert.h>
#include <stdio.h>
#include <es.h>
#include <es/tree.h>

/* Writes a dot graph of the tree.
 * cf. http://www.graphviz.org/
 */
template <class K, class V>
void dot(Tree<K, V>& tree)
{
    printf("digraph {\n");
    printf("    graph [rankdir=\"TB\"];\n");
    printf("\n");

    typename Tree<K, V>::Node* node;
    typename Tree<K, V>::Node* parent;
    typename Tree<K, V>::Iterator iter = tree.begin();
    while ((node = iter.next()))
    {
        printf("    \"%d,%d\"\n", node->getKey(), node->getLevel());
        parent = node->getParent();
        if (parent)
        {
            printf("    \"%d,%d\" -> \"%d,%d\"\n",
                   parent->getKey(), parent->getLevel(),
                   node->getKey(), node->getLevel());
        }
    }

    printf("}\n\n");
}

int main()
{
    Tree<int, int> a;
    ASSERT(a.isEmpty());
    ASSERT(a.getFirst() == 0);
    ASSERT(a.getLast() == 0);
    for (int i = 0; i < 7; ++i)
    {
        a.add(i, 0);
        dot(a);
    }
    ASSERT(!a.isEmpty());
    ASSERT(a.getFirst()->getKey() == 0);
    ASSERT(a.getLast()->getKey() == 6);

    int error = 0;
    try
    {
        a.add(1, 0);
    }
    catch (SystemException<EEXIST>& e)
    {
        error = e.getResult();
    }
    ASSERT(error == EEXIST);

    Tree<int, int> b;
    for (int i = 6; 0 <= i; --i)
    {
        b.add(i, 0);
        dot(b);
    }

    b.remove(0);
    dot(b);
    b.remove(3);
    dot(b);
    b.remove(1);
    dot(b);

    Tree<int, int>::Node* node;

    Tree<int, int>::Iterator fw = b.begin();
    while ((node = fw.next()))
    {
        printf("%d ", node->getKey());
    }
    printf("\n");

    Tree<int, int>::Iterator bw = b.end();
    while ((node = bw.previous()))
    {
        printf("%d ", node->getKey());
    }
    printf("\n");

    ASSERT(b.contains(2));
    ASSERT(!b.contains(0));

    printf("done.\n");
}
