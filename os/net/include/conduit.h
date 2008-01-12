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

#ifndef CONDUIT_H_INCLUDED
#define CONDUIT_H_INCLUDED

// cf. H. Hüni, R. Johnson, R. Engel,
//     A Framework for Network Protocol Software, ACM, 1995.

#include <string.h>
#include <es.h>
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
public: // XXX
    void*   chunk;
    long    len;
    long    position;
    int     type;       // type of data pointed by position

public:
    Messenger(void* chunk = 0, long len = 0, long pos = 0) :
        chunk(chunk),
        len(len),
        position(pos),
        type(0)
    {
    }
    virtual ~Messenger()
    {
    }

    virtual bool apply(Conduit* c)
    {
        return true;
    }

    long getSize() const
    {
        return len;
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

    long getLength() const
    {
        ASSERT(position <= len);
        return len - position;
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
        memmove(dst, static_cast<char*>(chunk) + offset, count);
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
        memmove(static_cast<char*>(chunk) + offset, src, count);
        return count;
    }

    void* fix(long count, long offset) const
    {
        if (offset < 0 || len <= offset || count <= 0 || offset + count < offset ||
            len < offset + count)
        {
            return 0;
        }
        return static_cast<char*>(chunk) + offset;
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
        return 0;
    };
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

    virtual bool accept(Messenger* m) = 0;
    virtual bool accept(Visitor* v, Conduit* sender = 0) = 0;
    virtual Conduit* clone(void* key)
    {
        return 0;
    }

    Receiver* getReceiver() const
    {
        return receiver;
    }
    void setReceiver(Receiver* receiver)
    {
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
        // at() method as sideB can be reset in the at() method.
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

public:
    ConduitFactory(Conduit* prototype = 0) :
        prototype(prototype)
    {
    }

    bool accept(Messenger* m)
    {
        return m->apply(this);
    }
    bool accept(Visitor* v, Conduit* sender = 0)
    {
        if (!v->at(this, sender))
        {
            return false;
        }
        if (sender != sideA)
        {
            return toA(v);
        }
        else
        {
            return toB(v);
        }
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
        if (!prototype)
        {
            return 0;
        }
        return new ConduitFactory(prototype->clone(key));
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
        if (!v->at(this, sender))
        {
            return false;
        }
        if (sender != sideA)
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
    ~Mux() {}

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
        if (!v->at(this, sender))
        {
            return false;
        }
        if (sender != sideA)
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

    Tree<void*, Conduit*>::Iterator list()
    {
        return sideB.begin();
    }

    Mux* clone(void* key)
    {
        Mux* m = new Mux(accessor, factory->clone(key));
        return m;
    }

    const char* getName() const
    {
        return "Mux";
    }
};

class Installer : public Visitor
{
    Conduit* conduit;

public:
    Installer(Messenger* messenger) :
        Visitor(messenger),
        conduit(0)
    {
    }

    bool at(ConduitFactory* f, Conduit* c)
    {
        if (conduit)
        {
            return true;
        }

        Mux* mux = dynamic_cast<Mux*>(c);
        if (mux)
        {
            ASSERT(mux->getFactory() == f);
            void* key = mux->getKey(getMessenger());
            conduit = f->create(key);
            if (conduit)
            {
                Conduit::connectAB(conduit, mux, key);
                return false;
            }
        }
        return true;
    }

    Conduit* getConduit() const
    {
        return conduit;
    }
};

class Uninstaller : public Visitor
{
    Conduit*    conduit;
    Mux*        mux;

    // Detach c from mux.
    void detach(Conduit* c)
    {
        ASSERT(c->getA() == mux);
        c->setA(0);
        mux->removeB(mux->getKey(getMessenger()));
        conduit = c;
    }

public:
    Uninstaller(Messenger* messenger) :
        Visitor(messenger),
        conduit(0),
        mux(0)
    {
    }

    bool at(Adapter* a, Conduit* c)
    {
        if (c && c == mux)
        {
            detach(a);
            return false;
        }
        return true;
    }

    bool at(Mux* m, Conduit* c)
    {
        if (mux == 0 && c == m->getA())
        {
            mux = m;
        }
    }

    bool at(Protocol* p, Conduit* c)
    {
        if (c && c == mux)
        {
            detach(p);
            return false;
        }
        return true;
    }

    bool at(ConduitFactory* f, Conduit* c)
    {
        return true;
    }

    Conduit* getConduit() const
    {
        return conduit;
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
    do
    {
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

esReport("transport: %p (%ld)\n", key, (long) key);

        do
        {
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
                // Try any key
                try
                {
                    Conduit* b = m->getB(0);
                    if (b->accept(this, m))
                    {
                        return true;
                    }
                }
                catch (SystemException<ENOENT>)
                {
                }
            }
        } while (!m->getFactory()->accept(this, m));    // Have factory added a new conduit to sideB?
        return false;
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
