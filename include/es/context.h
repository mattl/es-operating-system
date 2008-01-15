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

class Binding : public es::IBinding
{
    friend class Context;
    friend class Iterator;

    es::IMonitor*  monitor;
    Ref         ref;
    char*       name;
    IInterface* object;
    Context*    context;

    void hide();
    void detach();

public:
    Link<Binding>   link;

public:
    Binding(const char* name, IInterface* object);
    ~Binding();

    void* queryInterface(const Guid& riid);
    unsigned int addRef(void);
    unsigned int release(void);

    IInterface* getObject();
    void setObject(IInterface* object);
    int getName(char* name, int len);

    typedef List<Binding, &Binding::link> List;
};

// In Context, the first binding points to the Context itself named "".
class Context : public es::IContext
{
    friend class Binding;
    friend class Iterator;

    es::IMonitor*   monitor;
    Ref             ref;
    Binding::List   bindingList;

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

    void* queryInterface(const Guid& riid);
    unsigned int addRef(void);
    unsigned int release(void);

    es::IBinding* bind(const char* name, es::IInterface* object);
    es::IContext* createSubcontext(const char* name);
    int destroySubcontext(const char* name);
    es::IInterface* lookup(const char* name);
    int rename(const char* oldName, const char* newName);
    int unbind(const char* name);
    es::IIterator* list(const char* name);

    static const Guid& iid()
    {
        static const Guid iid =
        {
            0x8017f170, 0x1a13, 0x11dc, { 0x9c, 0x02, 0x00, 0x09, 0xbf, 0x00, 0x00, 0x01 }
        };
        return iid;
    }
};

class Iterator : public es::IIterator
{
    es::IMonitor*  monitor;
    Ref         ref;
    Binding*    binding;

public:
    Iterator(Binding* binding);
    ~Iterator();

    void* queryInterface(const Guid& riid);
    unsigned int addRef(void);
    unsigned int release(void);

    bool hasNext(void);
    es::IInterface* next();
    int remove(void);
};

#endif  // NINTENDO_ES_CONTEXT_H_INCLUDED
