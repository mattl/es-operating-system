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

#include <stdio.h>
#include <es.h>
#include <es/collection.h>

class S;
class A;

class S
{
    friend class A;

    char*   name;
    Collection<A*>  list;
public:
    S(char* name) : name(name) {}
    void join(A* a);
    void report();
    void remove(A* a)
    {
        list.remove(a);
    }
    void removeLast()
    {
        list.removeLast();
    }
    bool contains(A* a)
    {
        return list.contains(a);
    }
};

class A
{
    friend class S;

    char*   name;
    Collection<S*>  list;
public:
    A(char* name) : name(name) {}
    void report();
};

void S::join(A* a)
{
    list.addLast(a);
    a->list.addLast(this);
}

void S::report()
{
    printf("%s: ", name);
    Collection<A*>::Iterator iter(list.begin());
    while (iter.hasNext())
    {
        A* a = iter.next();
        printf("%s", a->name);
        if (iter.hasNext())
        {
            printf(", ");
        }
    }
    printf("\b\b\n");
}

void A::report()
{
    printf("%s: ", name);
    Collection<S*>::Iterator iter(list.begin());
    while (iter.hasNext())
    {
        S* s = iter.next();
        printf("%s", s->name);
        if (iter.hasNext())
        {
            printf(", ");
        }
    }
    printf("\b\b\n");
}

int main()
{
    S s1("x");
    S s2("y");
    S s3("z");

    A a1("a");
    A a2("b");
    A a3("c");

    s1.join(&a1);
    s2.join(&a1);
    s2.join(&a2);
    s2.join(&a3);

    s1.report();
    s2.report();
    s3.report();

    a1.report();
    a2.report();
    a3.report();

    s2.remove(&a2);
    s2.report();
    s2.removeLast();
    s2.report();
    printf("contains a: %d\n", s2.contains(&a1));
    printf("contatis b: %d\n", s2.contains(&a2));
    printf("contatis c: %d\n", s2.contains(&a3));

    printf("done.\n");
}
