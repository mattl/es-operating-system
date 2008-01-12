/*
 * Copyright (c) 2006, 2007
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

#include <string.h>
#include <es.h>
#include <es/clsid.h>
#include <es/handle.h>
#include <es/context.h>
#include "core.h"

//
// Binding Implementation
//

Binding::Binding(const char* name, IInterface* object) :
    monitor(0),
    object(object),
    context(0)
{
    esCreateInstance(CLSID_Monitor, IID_IMonitor,
                     reinterpret_cast<void**>(&monitor));
    size_t len = strlen(name);
    this->name = new char[len + 1];
    strcpy(this->name, name);
    if (object)
    {
        object->addRef();
    }
}

Binding::~Binding()
{
    if (context)
    {
        // Make sure release() is not called while context is locked.
        context->remove(this);
        context = 0;
    }
    if (object)
    {
        object->release();
    }
    if (name)
    {
        delete[] name;
    }
}

IInterface* Binding::getObject()
{
    Synchronized<IMonitor*> method(monitor);

    if (object)
    {
        object->addRef();
    }
    return object;
}

int Binding::setObject(IInterface* unknown)
{
    Synchronized<IMonitor*> method(monitor);

    if (object)
    {
        object->release();
    }
    if (unknown)
    {
        unknown->addRef();
        object = unknown;
    }
    return 0;
}

int Binding::getName(char* name, unsigned int len)
{
    Synchronized<IMonitor*> method(monitor);

    if (!this->name)
    {
        return -1;
    }
    strncpy(name, this->name, len);
    return 0;
}

// Makes this binding invisible from the context so that iterators can
// skip over this binding.
void Binding::hide()
{
    bool deleted(false);

    {
        Synchronized<IMonitor*> method(monitor);
        if (name)
        {
            delete[] name;
            name = 0;
            deleted = true;
        }
    }

    if (deleted)
    {
        release();      // for removal
    }
}

// Detachs this binding from the context.
void Binding::detach()
{
    Synchronized<IMonitor*> method(monitor);

    if (context)
    {
        // Make sure release() is not called while context is locked.
        context->remove(this);
        context = 0;
    }
}

bool Binding::queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IBinding)
    {
        *objectPtr = static_cast<IBinding*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IBinding*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Binding::addRef(void)
{
    return ref.addRef();
}

unsigned int Binding::release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}

//
// Context Implementation
//

bool Context::isDelimitor(int c)
{
    return (c == '/' || c == '\\') ? true : false;
}

bool Context::isPathname(const char* name)
{
    while (*name)
    {
        if (isDelimitor(*name))
        {
            return true;
        }
        ++name;
    }
    return false;
}

long Context::parse(const char* name, char* component, size_t len)
{
    if (len <= 0)
    {
        return -1;
    }

    const char* src;
    char* dst;
    for (src = name, dst = component; *src; ++src)
    {
        if (isDelimitor(*src))
        {
            while (isDelimitor(*++src))
            {
            }
            break;
        }
        if (dst - component < len - 1)
        {
            *dst++ = *src;
        }
    }
    *dst = 0;
    return src - name;
}

Context::Context() :
    monitor(0)
{
    esCreateInstance(CLSID_Monitor, IID_IMonitor,
                     reinterpret_cast<void**>(&monitor));
    Binding* binding = new Binding("", this);
    if (binding)
    {
        binding->context = this;
        bindingList.addLast(binding);
    }
    ASSERT(ref == 2);
}

Context::~Context()
{
    while (Binding* binding = getFirst())
    {
        binding->detach();
        binding->hide();
        binding->release();     // for getFirst()
    }
}

Binding* Context::getFirst()
{
    Synchronized<IMonitor*> method(monitor);

    Binding* binding = bindingList.getFirst();
    if (binding)
    {
        binding->addRef();
    }
    return binding;
}

Binding* Context::walk(const char* component)
{
    Synchronized<IMonitor*> method(monitor);

    Binding::List::Iterator iter = bindingList.begin();
    while (Binding* next = iter.next())
    {
        if (!next->name)
        {
            continue;
        }
        if (stricmp(component, next->name) == 0)
        {
            next->addRef();
            return next;
        }
    }
    return 0;
}

void Context::remove(Binding* binding)
{
    Synchronized<IMonitor*> method(monitor);

    if (binding && binding->context == this)
    {
        bindingList.remove(binding);
    }
}

// The reference count of the returned Binding object is incremented by one.
Binding* Context::walk(Context* context, const char*& name)
{
    if (!*name)
    {
        return context->getFirst();
    }

    context->addRef();
    Binding* binding(0);
    while (context && *name)
    {
        char component[256];
        long len = parse(name, component, sizeof component);
        if (component[0] == 0)
        {
            name += len;
            continue;
        }

        Binding* next(context->walk(component));
        if (!next)
        {
            break;
        }
        if (binding)
        {
            binding->release();
        }
        binding = next;
        name += len;

        IInterface* object = binding->getObject();
        if (!object)
        {
            break;
        }
        context->release();

        if (!object->queryInterface(IID_Context, (void**) &context))
        {
            context = 0;
        }
        object->release();
    }
    if (context)
    {
        context->release();
    }
    return binding;
}

IBinding* Context::bind(const char* name, IInterface* unknown)
{
    if (*name == 0)
    {
        return 0;   // "" is reserved for 'this'.
    }

    Binding* binding(walk(this, name));
    if (*name == 0)
    {
        if (binding)
        {
            binding->setObject(unknown);
        }
        return binding;
    }
    if (!binding)
    {
        if (isPathname(name))
        {
            return 0;
        }
        binding = new Binding(name, unknown);
        if (!binding)
        {
            return 0;
        }
        binding->context = this;
        bindingList.addLast(binding);
        binding->addRef();      // for the result
        return binding;
    }

    IInterface* object = binding->getObject();
    binding->release();
    Handle<IContext> hcontext(object);
    if (!hcontext)
    {
        return 0;
    }
    return hcontext->bind(name, unknown);
}

IInterface* Context::lookup(const char* name)
{
    Binding* binding(walk(this, name));
    if (!binding)
    {
        return 0;
    }

    IInterface* object = binding->getObject();
    binding->release();
    if (*name == 0)
    {
        return object;
    }

    Handle<IContext> hcontext(object);
    if (!hcontext)
    {
        return 0;
    }
    return hcontext->lookup(name);
}

int Context::rename(const char* oldName, const char* newName)
{
    if (*oldName == 0 || *newName == 0)
    {
        return -1;
    }

    // XXX

    // lookup oldName
    IInterface* object = lookup(oldName);
    if (!object)
    {
        return -1;
    }

    // bind object found to newName
    IBinding* binding = bind(newName, object);
    if (!binding)
    {
        object->release();
        return -1;
    }
    binding->release();

    // unbind oldName
    unbind(oldName);
    return object->release();
}

int Context::unbind(const char* name)
{
    if (*name == 0)
    {
        return -1;
    }

    Binding* binding(walk(this, name));
    if (!binding)
    {
        return -1;
    }

    if (*name == 0)
    {
        binding->hide();
        binding->release();     // for walk()
        return 0;
    }

    IInterface* object = binding->getObject();
    binding->release();
    Handle<IContext> hcontext(object);
    if (!hcontext)
    {
        return -1;
    }
    return hcontext->unbind(name);
}

IIterator* Context::list(const char* name)
{
    Binding* binding(walk(this, name));
    if (!binding)
    {
        return 0;
    }

    IInterface* object = binding->getObject();
    binding->release();
    if (!object)
    {
        return 0;
    }

    Context* context;
    if (!object->queryInterface(IID_Context, (void**) &context))
    {
        context = 0;
    }

    if (context)
    {
        binding = context->getFirst();
        IIterator* iterator = new ::Iterator(binding);
        binding->release();
        object->release();
        context->release();
        return iterator;
    }

    Handle<IContext> hcontext(object);
    if (!hcontext)
    {
        return 0;
    }
    return hcontext->list(name);
}

IContext* Context::createSubcontext(const char* name)
{
    if (*name == 0)
    {
        return 0;
    }

    Binding* binding(walk(this, name));
    if (*name == 0)
    {
        if (binding)
        {
            binding->release();
        }
        return 0;
    }
    if (!binding)
    {
        if (isPathname(name))
        {
            return 0;
        }
        Context* context = new Context;
        if (!context)
        {
            return 0;
        }
        binding = new Binding(name, context);
        if (!binding)
        {
            context->release();
            return 0;
        }
        binding->context = this;
        bindingList.addLast(binding);
        return context;
    }

    IInterface* object = binding->getObject();
    binding->release();
    Handle<IContext> hcontext(object);
    if (!hcontext)
    {
        return 0;
    }
    return hcontext->createSubcontext(name);
}

int Context::destroySubcontext(const char* name)
{
    if (*name == 0)
    {
        return -1;
    }

    Binding* binding(walk(this, name));
    if (!binding)
    {
        return -1;
    }

    IInterface* object = binding->getObject();
    Handle<IContext> hcontext(object);
    if (!hcontext)
    {
        binding->release();
        return -1;
    }
    if (*name == 0)
    {
        binding->hide();
        binding->release();
        return 0;
    }
    binding->release();
    return hcontext->destroySubcontext(name);
}

bool Context::queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IContext)
    {
        *objectPtr = static_cast<IContext*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IContext*>(this);
    }
    else if (riid == IID_Context)
    {
        *objectPtr = static_cast<Context*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Context::addRef(void)
{
    return ref.addRef();
}

unsigned int Context::release(void)
{
    unsigned int count = ref.release();
    if (count == 1)     // 1 for "" entry
    {
        delete this;
        return 0;
    }
    return count;
}

//
// Iterator Implementation
//

Iterator::Iterator(Binding* binding) :
    monitor(0),
    binding(binding)
{
    esCreateInstance(CLSID_Monitor, IID_IMonitor,
                     reinterpret_cast<void**>(&monitor));
    binding->addRef();
}

Iterator::~Iterator()
{
    if (binding)
    {
        binding->release();
    }
}

bool Iterator::hasNext(void)
{
    Synchronized<IMonitor*> method(monitor);

    ASSERT(binding->context);
    Context* context(binding->context);
    context->monitor->lock();
    Binding::List::Iterator iter = context->bindingList.list(binding);
    Binding* next;
    while ((next = iter.next()))
    {
        if (next->name)
        {
            context->monitor->unlock();
            return true;
        }
    }
    context->monitor->unlock();
    return false;
}

IInterface* Iterator::next()
{
    Synchronized<IMonitor*> method(monitor);

    ASSERT(binding->context);
    Context* context(binding->context);
    context->monitor->lock();
    Binding::List::Iterator iter = context->bindingList.list(binding);
    Binding* next;
    while ((next = iter.next()))
    {
        if (next->name)
        {
            next->addRef();     // for iterator
            next->addRef();     // for result
            context->monitor->unlock();
            binding->release(); // for iterator
            binding = next;
            return next;
        }
    }
    context->monitor->unlock();
    return 0;
}

int Iterator::remove(void)
{
    Synchronized<IMonitor*> method(monitor);

    ASSERT(binding->context);
    Context* context(binding->context);
    context->monitor->lock();
    Binding::List::Iterator iter = context->bindingList.list(binding);
    Binding* prev = iter.previous();
    if (!prev)
    {
        // The first object is context itself which must not be removed.
        context->monitor->unlock();
        return -1;
    }
    prev->addRef(); // for iterator
    context->monitor->unlock();

    binding->hide();
    binding->release();     // for iterator
    binding = prev;
    return 0;
}

bool Iterator::queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IIterator)
    {
        *objectPtr = static_cast<IIterator*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<IIterator*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
}

unsigned int Iterator::addRef(void)
{
    return ref.addRef();
}

unsigned int Iterator::release(void)
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
