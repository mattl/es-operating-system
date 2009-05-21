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

#ifndef NINTENDO_ES_CONTEXT_H_INCLUDED
#define NINTENDO_ES_CONTEXT_H_INCLUDED

#include <es/handle.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/synchronized.h>
#include <es/base/IMonitor.h>
#include <es/naming/IContext.h>
#include <es/naming/IBinding.h>

class Binding;
class Context;
class Iterator;

class Binding : public es::Binding
{
    friend class Context;
    friend class Iterator;

    es::Monitor*   monitor;
    Ref            ref;
    char*          name;
    Object* object;
    Context*       context;

    void hide();
    void detach();

public:
    Link<Binding>   link;

public:
    Binding(const char* name, Object* object);
    ~Binding();

    Object* queryInterface(const char* iid);
    unsigned int addRef();
    unsigned int release();

    Object* getObject();
    void setObject(Object* object);
    const char* getName(void* name, int nameLength);

    typedef ::List<Binding, &Binding::link> List;
};

// In Context, the first binding points to the Context itself named "".
class Context : public es::Context
{
    friend class Binding;
    friend class Iterator;

    es::Monitor*  monitor;
    Ref           ref;
    Binding::List bindingList;

    static bool isDelimitor(int c);
    static bool isPathname(const char* name);
    static long parse(const char* name, char* component, size_t len);
    static Binding* walk(Context* context, const char*& name);

    Binding* getFirst();
    Binding* walk(const char* component);
    void remove(Binding* binding);

public:
    Context();
    ~Context();

    Object* queryInterface(const char* iid);
    unsigned int addRef();
    unsigned int release();

    es::Binding* bind(const char* name, Object* object);
    es::Context* createSubcontext(const char* name);
    int destroySubcontext(const char* name);
    Object* lookup(const char* name);
    int rename(const char* oldName, const char* newName);
    int unbind(const char* name);
    es::Iterator* list(const char* name);

    static const char* iid()
    {
        static const char* name = "__Context";
        return name;
    }
};

class Iterator : public es::Iterator
{
    es::Monitor* monitor;
    Ref          ref;
    Binding*     binding;

public:
    Iterator(Binding* binding);
    ~Iterator();

    Object* queryInterface(const char* iid);
    unsigned int addRef();
    unsigned int release();

    bool hasNext();
    Object* next();
    int remove();
};

#endif  // NINTENDO_ES_CONTEXT_H_INCLUDED
