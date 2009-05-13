/*
 * Copyright 2008, 2009 Google Inc.
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

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/context.h>
#include <es/base/IMonitor.h>
#include "core.h"

//
// Binding Implementation
//

Binding::Binding(const char* name, Object* object) :
    monitor(0),
    object(object),
    context(0)
{
    monitor = es::Monitor::createInstance();
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

Object* Binding::getObject()
{
    Synchronized<es::Monitor*> method(monitor);

    if (object)
    {
        object->addRef();
    }
    return object;
}

void Binding::setObject(Object* unknown)
{
    Synchronized<es::Monitor*> method(monitor);

    if (object)
    {
        object->release();
    }
    if (unknown)
    {
        unknown->addRef();
        object = unknown;
    }
}

const char* Binding::getName(void* name, int len)
{
    Synchronized<es::Monitor*> method(monitor);

    if (!this->name)
    {
        return 0;
    }
    strncpy(static_cast<char*>(name), this->name, len);
    return static_cast<char*>(name);
}

// Makes this binding invisible from the context so that iterators can
// skip over this binding.
void Binding::hide()
{
    bool deleted(false);

    {
        Synchronized<es::Monitor*> method(monitor);
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
    Synchronized<es::Monitor*> method(monitor);

    if (context)
    {
        // Make sure release() is not called while context is locked.
        context->remove(this);
        context = 0;
    }
}

Object* Binding::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Binding::iid()) == 0)
    {
        objectPtr = static_cast<es::Binding*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Binding*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Binding::addRef()
{
    return ref.addRef();
}

unsigned int Binding::release()
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
    monitor = es::Monitor::createInstance();
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
    Synchronized<es::Monitor*> method(monitor);

    Binding* binding = bindingList.getFirst();
    if (binding)
    {
        binding->addRef();
    }
    return binding;
}

Binding* Context::walk(const char* component)
{
    Synchronized<es::Monitor*> method(monitor);

    Binding::List::Iterator iter = bindingList.begin();
    while (Binding* next = iter.next())
    {
        if (!next->name)
        {
            continue;
        }
        if (strcasecmp(component, next->name) == 0)
        {
            next->addRef();
            return next;
        }
    }
    return 0;
}

void Context::remove(Binding* binding)
{
    Synchronized<es::Monitor*> method(monitor);

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

        Object* object = binding->getObject();
        if (!object)
        {
            break;
        }
        context->release();

        context = reinterpret_cast<Context*>(object->queryInterface(Context::iid()));
        object->release();
    }
    if (context)
    {
        context->release();
    }
    return binding;
}

es::Binding* Context::bind(const char* name, Object* unknown)
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

    Object* object = binding->getObject();
    binding->release();
    Handle<es::Context> hcontext(object);
    if (!hcontext)
    {
        return 0;
    }
    return hcontext->bind(name, unknown);
}

Object* Context::lookup(const char* name)
{
    Binding* binding(walk(this, name));
    if (!binding)
    {
        return 0;
    }

    Object* object = binding->getObject();
    binding->release();
    if (*name == 0)
    {
        return object;
    }

    Handle<es::Context> hcontext(object);
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
    Object* object = lookup(oldName);
    if (!object)
    {
        return -1;
    }

    // bind object found to newName
    es::Binding* binding = bind(newName, object);
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

    Object* object = binding->getObject();
    binding->release();
    Handle<es::Context> hcontext(object);
    if (!hcontext)
    {
        return -1;
    }
    return hcontext->unbind(name);
}

es::Iterator* Context::list(const char* name)
{
    Binding* binding(walk(this, name));
    if (!binding)
    {
        return 0;
    }

    Object* object = binding->getObject();
    binding->release();
    if (!object)
    {
        return 0;
    }

    Context* context = reinterpret_cast<Context*>(object->queryInterface(Context::iid()));

    if (context)
    {
        binding = context->getFirst();
        es::Iterator* iterator = new ::Iterator(binding);
        binding->release();
        object->release();
        context->release();
        return iterator;
    }

    Handle<es::Context> hcontext(object);
    if (!hcontext)
    {
        return 0;
    }
    return hcontext->list(name);
}

es::Context* Context::createSubcontext(const char* name)
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

    Object* object = binding->getObject();
    binding->release();
    Handle<es::Context> hcontext(object);
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

    Object* object = binding->getObject();
    Handle<es::Context> hcontext(object);
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

Object* Context::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Context::iid()) == 0)
    {
        objectPtr = static_cast<es::Context*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Context*>(this);
    }
    else if (strcmp(riid, Context::iid()) == 0)
    {
        objectPtr = static_cast<Context*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Context::addRef()
{
    return ref.addRef();
}

unsigned int Context::release()
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
    monitor = es::Monitor::createInstance();
    binding->addRef();
}

Iterator::~Iterator()
{
    if (binding)
    {
        binding->release();
    }
}

bool Iterator::hasNext()
{
    Synchronized<es::Monitor*> method(monitor);

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

Object* Iterator::next()
{
    Synchronized<es::Monitor*> method(monitor);

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

int Iterator::remove()
{
    Synchronized<es::Monitor*> method(monitor);

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

Object* Iterator::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Iterator::iid()) == 0)
    {
        objectPtr = static_cast<es::Iterator*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Iterator*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Iterator::addRef()
{
    return ref.addRef();
}

unsigned int Iterator::release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
