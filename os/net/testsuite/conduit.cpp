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

#include <stdio.h>
#include <string.h>
#include <new>
#include <es.h>
#include "conduit.h"

class InputReceiver : public Receiver
{
public:
    virtual bool input(Messenger* m) = 0;
};

class InputMessenger : public Messenger
{
public:
    InputMessenger(char* message) :
        Messenger(message, strlen(message))
    {
    }
    virtual bool apply(Conduit* c)
    {
        InputReceiver* receiver = dynamic_cast<InputReceiver*>(c->getReceiver());
        if (receiver)
        {
            return receiver->input(this);
        }
        return Messenger::apply(c);
    }
};

class OutputReceiver : public Receiver
{
public:
    virtual bool output(Messenger* m) = 0;
};

class OutputMessenger : public Messenger
{
public:
    OutputMessenger(char* message) :
        Messenger(message, strlen(message))
    {
    }
    virtual bool apply(Conduit* c)
    {
        OutputReceiver* receiver = dynamic_cast<OutputReceiver*>(c->getReceiver());
        if (receiver)
        {
            return receiver->output(this);
        }
        return Messenger::apply(c);
    }
};

class Ethernet : public OutputReceiver
{
public:
    bool output(Messenger* m)
    {
        printf("'%.*s'\n", m->len, m->chunk);
        return true;
    }
};

class Socket : public InputReceiver
{
    char port;
public:
    Socket(char port) : port(port)
    {
    }
    bool input(Messenger* m)
    {
        printf("'%.*s'\n", m->len, m->chunk);
        return true;
    }
};

class PortAccessor : public Accessor
{
public:
    void* getKey(Messenger* m)
    {
        ASSERT(m);
        char port;

        m->read(&port, 1, 0);
        return reinterpret_cast<void*>(port);
    }
};

class SocketInstaller : public Visitor
{
    Adapter* socketAdapter;

public:
    SocketInstaller(Messenger* t, Adapter* socketAdapter) :
        Visitor(t),
        socketAdapter(socketAdapter)
    {
    }
    bool at(ConduitFactory* f, Conduit* c)
    {
        Mux* m = dynamic_cast<Mux*>(c);
        if (m)
        {
            ASSERT(m->getFactory() == f);
            void* key = m->getKey(getMessenger());
            Conduit* e = f->create(key);
            if (e)
            {
                Conduit::connectAB(socketAdapter, e);
                Conduit::connectAB(e, m, key);
                return false;
            }
        }
        return true;
    }
};

class Visualizer : public Visitor
{
public:
    Visualizer() :
        Visitor(0)
    {
        printf("digraph {\n");
        printf("    graph [rankdir=\"BT\"];\n");
        printf("    edge [fontsize=7, fontname=Lucon];\n");
        printf("    node [fontsize=7, fontname=Lucon];\n");
        printf("\n");
    }
    ~Visualizer()
    {
        printf("}\n");
    }
    bool at(Adapter* a, Conduit* c)
    {
        printf("    %s_%p [shape=ellipse]\n", a->getName(), a);
        if (c)
        {
            printf("    %s_%p -> %s_%p;\n", c->getName(), c, a->getName(), a);
            return false;
        }
        return true;
    }
    bool at(Mux* m, Conduit* c)
    {
        printf("    %s_%p ", m->getName(), m);
        if (c == m->getA())
        {
            printf("[shape=invtrapezium]\n");
        }
        else
        {
            printf("[shape=trapezium]\n");
        }
        if (c)
        {
            printf("    %s_%p -> %s_%p;\n", c->getName(), c, m->getName(), m);
        }
        return true;
    }
    bool at(Protocol* p, Conduit* c)
    {
        printf("    %s_%p [shape=box]\n", p->getName(), p);
        if (c)
        {
            printf("    %s_%p -> %s_%p;\n", c->getName(), c, p->getName(), p);
        }
        return true;
    }
    bool at(ConduitFactory* f, Conduit* c)
    {
        printf("    %s_%p [shape=box]\n", f->getName(), f);
        if (c)
        {
            printf("    %s_%p -> %s_%p;\n", c->getName(), c, f->getName(), f);
        }
        return true;
    }
};

int main()
{
    Ethernet    ether;
    Adapter     nic;
    nic.setReceiver(&ether);

    Protocol        prototype;
    ConduitFactory  factory(&prototype);
    PortAccessor    accessor;
    Mux             mux(&accessor, &factory);

    Conduit::connectAA(&mux, &nic);

    char c1[] = "1";
    Messenger   t1(c1, 1);
    Socket      s1('1');
    Adapter     a1;
    a1.setReceiver(&s1);
    SocketInstaller installer1(&t1, &a1);
    nic.accept(&installer1);

    {
        Visualizer vi;
        nic.accept(&vi);
    }

    bool result;

    InputMessenger x("1:hello.");
    Visitor vx(&x);
    result = nic.accept(&vx);
    ASSERT(result);

    InputMessenger y("2:hello.");
    Visitor vy(&y);
    result = nic.accept(&vy);
    ASSERT(!result);

    char c2[] = "2";
    Messenger   t2(c2, 1);
    Socket      s2('2');
    Adapter     a2;
    a2.setReceiver(&s2);
    SocketInstaller installer2(&t2, &a2);
    nic.accept(&installer2);

    {
        Visualizer vi;
        nic.accept(&vi);
    }

    result = nic.accept(&vy);
    ASSERT(result);

    OutputMessenger z("hi.");
    Visitor vo(&z);
    a1.accept(&vo);
}
