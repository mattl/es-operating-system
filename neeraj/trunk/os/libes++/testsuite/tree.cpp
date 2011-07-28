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
