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

#ifndef CONDUIT_H_INCLUDED
#define CONDUIT_H_INCLUDED

// cf. H. Hüni, R. Johnson, R. Engel,
//     A Framework for Network Protocol Software, ACM, 1995.

#include <string.h>
#include <es.h>
#include <es/ref.h>
#include <es/types.h>
#include <es/tree.h>

class Accessor;
class Adapter;
class Conduit;
class Messenger;
class Mux;
class Protocol;
class ConduitFactory;
class Receiver;
class Visitor;

class Messenger
{
    Ref     ref;

    char*   chunk;
    long    len;
    long    position;
    int     type;       // type of data pointed by position

    long    saved;      // saved position
    bool    internal;   // true if chunk is allocated internally

public:
    Messenger(long len = 0, long pos = 0, void* chunk = 0) :
        chunk(static_cast<char*>(chunk)),
        len(len),
        position(pos),
        type(0),
        saved(0),
        internal(false)
    {
        ASSERT(0 <= len);
        if (0 < len && this->chunk == 0)
        {
            this->chunk = new char[len];
            internal = true;
        }
    }
    virtual ~Messenger()
    {
        ASSERT(0 <= len);
        if (internal)
        {
            delete[] chunk;
        }
    }

    virtual bool apply(Conduit* c)
    {
        return true;
    }

   void dump(const char* header, ...)
   {
        va_list list;
        va_start(list,header);

        esReport("---\n");
        esReportv(header,list);
        for (int i = 0; i < len && len > 0; i++)
        {
            if (i % 16 == 0)
            {
                esReport("\n");
            }
            esReport("%02x ", (u8)chunk[i]);
        }
        esReport("\n---\n");
        return;
    }

    long getSize() const
    {
        return len;
    }
    void setSize(long len)
    {
        this->len = len;
    }

    long getPosition() const
    {
        return position;
    }
    void setPosition(long pos)
    {
        if (pos < 0 || len < pos)
        {
            return;
        }
        position = pos;
    }
    long movePosition(long delta)
    {
        if (delta < 0)
        {
            if (delta < -position)
            {
                return position;
            }
        }
        else if (position + delta < position)
        {
            return position;
        }
        position += delta;
        return position;
    }

    void savePosition()
    {
        saved = position;
    }
    void restorePosition()
    {
        position = saved;
    }

    long getLength() const
    {
        ASSERT(position <= len);
        return len - position;
    }
    void setLength(long len)
    {
        ASSERT(0 <= len && position + len <= this->len);
        this->len = position + len;
    }

    int getType() const
    {
        return type;
    }
    void setType(int type)
    {
        this->type = type;
    }

    int read(void* dst, int count, long offset)
    {
        if (offset < 0 || len <= offset || count <= 0 || offset + count < offset)
        {
            return 0;
        }
        if (len < offset + count)
        {
            count = len - offset;
        }
        memmove(dst, chunk + offset, count);
        return count;
    }
    int write(const void* src, int count, long offset)
    {
        if (offset < 0 || len <= offset || count <= 0 || offset + count < offset)
        {
            return 0;
        }
        if (len < offset + count)
        {
            count = len - offset;
        }
        memmove(chunk + offset, src, count);
        return count;
    }

    void* fix(long count, long offset) const
    {
        if (offset < 0 || len <= offset || count <= 0 || offset + count < offset ||
            len < offset + count)
        {
            return 0;
        }
        return chunk + offset;
    }
    void* fix(long count) const
    {
        return fix(count, getPosition());
    }

    s32 sumUp(long count) const
    {
        register u16* ptr = static_cast<u16*>(fix(count));
        register s32 sum = 0;

        while (1 < count)
        {
            // This is the inner loop
            sum += *ptr++;
            count -= 2;
        }

        // Add left-over byte, if any
        if (0 < count)
        {
            sum += *reinterpret_cast<u8*>(ptr);
        }

        return sum;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
        }
        return count;
    }
};

class Visitor
{
    Messenger*  messenger;

public:
    Visitor(Messenger* messenger) :
        messenger(messenger)
    {
    }
    virtual ~Visitor()
    {
    }

    Messenger* getMessenger() const
    {
        return messenger;
    }

    virtual bool at(Adapter* a, Conduit* c);
    virtual bool at(Mux* m, Conduit* c);
    virtual bool at(Protocol* p, Conduit* c);
    virtual bool at(ConduitFactory* f, Conduit* c);

    virtual bool toB(Mux* m);
};

class Accessor
{
public:
    virtual void* getKey(Messenger* m) = 0;
};

class Receiver
{
public:
    Receiver() {}
    virtual ~Receiver() {}
    virtual Receiver* clone(Conduit* conduit, void* key)
    {
        return this;
    };
    virtual unsigned int release()
    {
        return 1;
    }
};

class Conduit
{
protected:
    Receiver*       receiver;
    Conduit*        sideA;
    Conduit*        sideB;

public:
    Conduit() :
        receiver(0),
        sideA(0),
        sideB(0)
    {
    }
    virtual ~Conduit()
    {
    }

    virtual unsigned int release()
    {
        if (receiver)
        {
            receiver->release();
            receiver = 0;
        }
        delete this;
        return 0;
    }

    virtual bool accept(Messenger* m) = 0;
    virtual bool accept(Visitor* v, Conduit* sender = 0) = 0;
    virtual Conduit* clone(void* key) = 0;

    Receiver* getReceiver() const
    {
        return receiver;
    }
    void setReceiver(Receiver* receiver)
    {
        ASSERT(receiver);
        this->receiver = receiver;
    }

    bool toA(Visitor* v)
    {
        if (sideA)
        {
            return sideA->accept(v, this);
        }
        return true;
    }
    bool toB(Visitor* v)
    {
        if (sideB)
        {
            return sideB->accept(v, this);
        }
        return true;
    }

    virtual void setA(Conduit* c)
    {
        sideA = c;
    }
    virtual void setB(Conduit* c)
    {
        sideB = c;
    }

    virtual Conduit* getA() const
    {
        return sideA;
    }
    virtual Conduit* getB() const
    {
        return sideB;
    }

    virtual bool isEmpty() const
    {
        return sideB ? false : true;
    }

    virtual const char* getName() const
    {
        return "Conduit";
    }

    static void connectAA(Conduit* x, Conduit* y)
    {
        x->setA(y);
        y->setA(x);
    }

    static void connectAB(Conduit* x, Conduit* y)
    {
        x->setA(y);
        y->setB(x);
    }

    static void connectBA(Conduit* x, Conduit* y)
    {
        x->setB(y);
        y->setA(x);
    }

    static void connectBB(Conduit* x, Conduit* y)
    {
        x->setB(y);
        y->setB(x);
    }

    static void connectAB(Conduit* x, Mux* y, void* keyX);
    static void connectBA(Mux* x, Conduit* y, void* keyY);
    static void connectBB(Mux* x, Mux* y, void* keyX, void* keyY);
};

class Protocol : public Conduit
{
public:
    bool accept(Messenger* m)
    {
        return m->apply(this);
    }
    bool accept(Visitor* v, Conduit* sender = 0)
    {
        // Determine the direction to forward the messenger before calling the
        // at() method as sideB can be reset in the at() method. Note the
        // default direction is to B.
        bool (Protocol::*to)(Visitor*);
        to = (sender == sideB) ? &Protocol::toA : &Protocol::toB;
        if (!v->at(this, sender))
        {
            return false;
        }
        (this->*to)(v);
    }

    Protocol* clone(void* key)
    {
        Protocol* p = new Protocol();
        if (receiver)
        {
            p->setReceiver(receiver->clone(p, key));
        }
        return p;
    }

    const char* getName() const
    {
        return "Protocol";
    }
};

class ConduitFactory : public Conduit
{
    Conduit*    prototype;
    void*       defaultKey;
    bool        hasDefault;

public:
    ConduitFactory(Conduit* prototype = 0) :
        prototype(prototype),
        defaultKey(0),
        hasDefault(false)
    {
    }

    unsigned int release()
    {
        if (prototype)
        {
            prototype->release();
            prototype = 0;
        }
        return Conduit::release();
    }

    bool accept(Messenger* m)
    {
        return m->apply(this);
    }
    bool accept(Visitor* v, Conduit* sender = 0)
    {
        bool (ConduitFactory::*to)(Visitor*);
        to = (sender == sideB) ? &ConduitFactory::toA : &ConduitFactory::toB;
        if (!v->at(this, sender))
        {
            return false;
        }
        (this->*to)(v);
    }
    Conduit* create(void* key)
    {
        if (!prototype)
        {
            return 0;
        }
        return prototype->clone(key);
    }

    ConduitFactory* clone(void* key)
    {
        ConduitFactory* f = new ConduitFactory(prototype ? prototype->clone(key) : 0);
        if (receiver)
        {
            f->setReceiver(receiver->clone(f, key));
        }
        if (hasDefaultKey())
        {
            f->addDefaultKey(getDefaultKey());
        }
        return f;
    }

    bool hasDefaultKey() const
    {
        return hasDefault;
    }

    void* getDefaultKey() const
    {
        ASSERT(hasDefault);
        return defaultKey;
    }

    void addDefaultKey(void* key)
    {
        hasDefault = true;
        defaultKey = key;
    }

    void removeDefaultKey(void* key)
    {
        ASSERT(key == defaultKey);
        hasDefault = false;
        defaultKey = 0;
    }

    const char* getName() const
    {
        return "ConduitFactory";
    }
};

class Adapter : public Conduit
{
    Conduit* getB() const
    {
        return 0;
    }
    void setB(Conduit* c)
    {
    }

public:
    bool accept(Messenger* m)
    {
        return m->apply(this);
    }
    bool accept(Visitor* v, Conduit* sender = 0)
    {
        Conduit* conduit = sideA;
        if (!v->at(this, sender))
        {
            return false;
        }
        if (sender != conduit)
        {
            return toA(v);
        }
        else
        {
            return true;
        }
    }

    Adapter* clone(void* key)
    {
        Adapter* a = new Adapter();
        if (receiver)
        {
            a->setReceiver(receiver->clone(a, key));
        }
        return a;
    }

    const char* getName() const
    {
        return "Adapter";
    }
};

class Mux : public Conduit
{
    Accessor*               accessor;
    ConduitFactory*         factory;
    Tree<void*, Conduit*>   sideB;

    Conduit* getB() const
    {
        return 0;
    }
    void setB(Conduit* c)
    {
    }

public:
    Mux(Accessor* a, ConduitFactory* f) :
        accessor(a),
        factory(f)
    {
        factory->setA(this);
    }
    ~Mux()
    {
    }

    unsigned int release()
    {
        if (factory)
        {
            factory->release();
            factory = 0;
        }
        return Conduit::release();
    }

    ConduitFactory* getFactory() const
    {
        return factory;
    }

    void* getKey(Messenger* m)
    {
        if (m)
        {
            return accessor->getKey(m);
        }
        else
        {
            return 0;
        }
    }
    Accessor* getAccessor() const
    {
        return accessor;
    }

    bool accept(Messenger* m)
    {
        return m->apply(this);
    }
    bool accept(Visitor* v, Conduit* sender = 0)
    {
        Conduit* conduit = sideA;
        if (!v->at(this, sender))
        {
            return false;
        }
        if (sender != conduit)
        {
            ASSERT(sideA);
            ASSERT(sender);
            return toA(v);
        }
        else
        {
            return v->toB(this);
        }
    }
    void addB(void* key, Conduit* c)
    {
        sideB.add(key, c);
    }
    void removeB(void* key)
    {
        sideB.remove(key);
    }

    bool isEmpty() const
    {
        return sideB.isEmpty();
    }

    Conduit* getB(void* key) const
    {
        return sideB.get(key);
    }

    bool contains(void* key) const
    {
        return sideB.contains(key);
    }

    Tree<void*, Conduit*>::Iterator list()
    {
        return sideB.begin();
    }

    Mux* clone(void* key)
    {
        Mux* m = new Mux(accessor, factory->clone(key));
        if (receiver)
        {
            m->setReceiver(receiver->clone(m, key));
        }
        return m;
    }

    const char* getName() const
    {
        return "Mux";
    }

    void* getKey(Conduit* b)
    {
        Tree<void*, Conduit*>::Node* node;
        Tree<void*, Conduit*>::Iterator iter = list();
        while ((node = iter.next()))
        {
            if (node->getValue() == b)
            {
                return node->getKey();
            }
        }
        return 0;
    }
};

class Installer : public Visitor
{
public:
    Installer(Messenger* messenger) :
        Visitor(messenger)
    {
    }

    bool at(Adapter* a, Conduit* c)
    {
        return true;
    }

    bool at(Mux* m, Conduit* c)
    {
        return true;
    }

    bool at(Protocol* p, Conduit* c)
    {
        return true;
    }

    bool at(ConduitFactory* f, Conduit* c)
    {
        Mux* mux = dynamic_cast<Mux*>(c);
        ASSERT(mux);
        ASSERT(mux->getFactory() == f);
        void* key = mux->getKey(getMessenger());
        if (Conduit* conduit = f->create(key))
        {
            Conduit::connectAB(conduit, mux, key);
            return false;
        }
        return true;
    }
};

class Uninstaller : public Visitor
{
public:
    Uninstaller(Messenger* messenger) :
        Visitor(messenger)
    {
    }

    bool at(Adapter* a, Conduit* c)
    {
        return true;
    }

    bool at(Mux* mux, Conduit* c)
    {
        if (c->isEmpty())
        {
            c->setA(0);
            mux->removeB(mux->getKey(getMessenger()));
            c->release();
            return true;
        }
        return false;       // To stop this visitor
    }

    bool at(Protocol* p, Conduit* c)
    {
        return false;       // To stop this visitor
    }

    bool at(ConduitFactory* f, Conduit* c)
    {
        return true;
    }
};

inline bool Visitor::at(Adapter* a, Conduit* c)
{
    return messenger->apply(a);
}

inline bool Visitor::at(Mux* m, Conduit* c)
{
    return messenger->apply(m);
}

inline bool Visitor::at(Protocol* p, Conduit* c)
{
    return messenger->apply(p);
}

inline bool Visitor::at(ConduitFactory* f, Conduit* c)
{
    return messenger->apply(f);
}

inline bool Visitor::toB(Mux* m)
{
    void* key = m->getKey(getMessenger());
    do {
        try
        {
            Conduit* b = m->getB(key);
            if (b->accept(this, m))
            {
                return true;
            }
        }
        catch (SystemException<ENOENT>)
        {
        }
    } while (!m->getFactory()->accept(this, m));    // Have factory added a new conduit to sideB?
    return false;
}

class BroadcastVisitor : public Visitor
{
public:
    BroadcastVisitor(Messenger* messenger) :
        Visitor(messenger)
    {
    }

    bool toB(Mux* m)
    {
        Tree<void*, Conduit*>::Node* node;
        Tree<void*, Conduit*>::Iterator iter = m->list();
        while ((node = iter.next()))
        {
            Conduit* b = node->getValue();
            b->accept(this, m);
        }
        return true;
    }
};

// Transporter extends Visitor to visit conduits using the default factory keys
// if no matches are found.
class Transporter : public Visitor
{
public:
    Transporter(Messenger* messenger) :
        Visitor(messenger)
    {
    }

    bool toB(Mux* m)
    {
        void* key = m->getKey(getMessenger());
        ConduitFactory* factory = m->getFactory();

        esReport("transport: %p (%ld)\n", key, (long) key);

        do {
            try
            {
                Conduit* b = m->getB(key);
                if (b->accept(this, m))
                {
                    return true;
                }
            }
            catch (SystemException<ENOENT>)
            {
                if (factory->hasDefaultKey())
                {
                    try // with default key
                    {
                        Conduit* b = m->getB(factory->getDefaultKey());
                        if (b->accept(this, m))
                        {
                            return true;
                        }
                    }
                    catch (SystemException<ENOENT>)
                    {
                    }
                }
            }
        } while (!factory->accept(this, m));    // Have factory added a new conduit to sideB?
        return false;
    }
};

class TypeAccessor : public Accessor
{
public:
    void* getKey(Messenger* m)
    {
        return reinterpret_cast<void*>(m->getType());
    }
};

inline void Conduit::
connectAB(Conduit* x, Mux* y, void* keyX)
{
    x->setA(y);
    y->addB(keyX, x);
}

inline void Conduit::
connectBA(Mux* x, Conduit* y, void* keyY)
{
    x->addB(keyY, y);
    y->setA(x);
}

inline void Conduit::
connectBB(Mux* x, Mux* y, void* keyX, void* keyY)
{
    x->addB(keyY, y);
    y->addB(keyX, x);
}

#endif // CONDUIT_H_INCLUDED
