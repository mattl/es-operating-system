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

#ifndef VISUALIZER_H_INCLUDED
#define VISUALIZER_H_INCLUDED

#include <es.h>
#include "conduit.h"

class Visualizer : public BroadcastVisitor
{
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
            esReport("    %s_%p -> %s_%p;\n", c->getName(), c, a->getName(), a);
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
            esReport("    %s_%p -> %s_%p;\n", c->getName(), c, m->getName(), m);
        }
        return true;
    }
    bool at(Protocol* p, Conduit* c)
    {
        esReport("    %s_%p [shape=box]\n", p->getName(), p);
        if (c)
        {
            esReport("    %s_%p -> %s_%p;\n", c->getName(), c, p->getName(), p);
            if (c == p->getA())
            {
                if (!p->getB())
                {
                    return false;
                }
            }
            else if (!p->getA())
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
