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

#ifndef VISUALIZER_H_INCLUDED
#define VISUALIZER_H_INCLUDED

#include <es.h>
#include "conduit.h"

class Visualizer : public BroadcastVisitor
{
    void edge(Conduit* c, Conduit* p)
    {
        Mux* m = dynamic_cast<Mux*>(c);
        if (!m)
        {
            esReport("    %s_%p -> %s_%p;\n", c->getName(), c, p->getName(), p);
        }
        else
        {
            long key = (long) m->getKey(p);
            if (0 <= key && key < 65536)
            {
                esReport("    %s_%p -> %s_%p [label=\"%ld\"];\n",
                         c->getName(), c, p->getName(), p, key);
            }
            else
            {
                esReport("    %s_%p -> %s_%p [label=\"0x%p\"];\n",
                         c->getName(), c, p->getName(), p, key);
            }
        }
    }

public:
    Visualizer() :
        BroadcastVisitor(0)
    {
        esReport("digraph {\n");
        esReport("    graph [rankdir=\"BT\"];\n");
        esReport("    edge [fontsize=7, fontname=Lucon];\n");
        esReport("    node [fontsize=7, fontname=Lucon];\n");
        esReport("\n");
    }
    ~Visualizer()
    {
        esReport("}\n");
    }
    bool at(Adapter* a, Conduit* c)
    {
        esReport("    %s_%p [shape=ellipse]\n", a->getName(), a);
        if (c)
        {
            edge(c, a);
            return false;
        }
        return true;
    }
    bool at(Mux* m, Conduit* c)
    {
        esReport("    %s_%p ", m->getName(), m);
        if (c == m->getA())
        {
            esReport("[shape=invtrapezium]\n");
        }
        else
        {
            esReport("[shape=trapezium]\n");
        }
        if (c)
        {
            edge(c, m);
        }
        return true;
    }
    bool at(Protocol* p, Conduit* c)
    {
        esReport("    %s_%p [shape=box]\n", p->getName(), p);
        if (c)
        {
            edge(c, p);
            if (c == p->getA())
            {
                if (!p->getB())
                {
                    return false;
                }
            }
            else if (c == p->getB())
            {
                if (!p->getA())
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        return true;
    }
    bool at(ConduitFactory* f, Conduit* c)
    {
        esReport("    %s_%p [shape=box]\n", f->getName(), f);
        if (c)
        {
            esReport("    %s_%p -> %s_%p;\n", c->getName(), c, f->getName(), f);
        }
        return true;
    }
};

#endif // VISUALIZER_H_INCLUDED
