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

static const Guid IID_Context =
{
    0x8017f170, 0x1a13, 0x11dc, { 0x9c, 0x02, 0x00, 0x09, 0xbf, 0x00, 0x00, 0x01 }
};

class Binding : public IBinding
{
    friend class Context;
    friend class Iterator;

    IMonitor*   monitor;
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

    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);

    IInterface* getObject();
    int setObject(IInterface* object);
    int getName(char* name, unsigned int len);

    typedef List<Binding, &Binding::link> List;
};

// In Context, the first binding points to the Context itself named "".
class Context : public IContext
{
    friend class Binding;
    friend class Iterator;

    IMonitor*       monitor;
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

    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);

    IBinding* bind(const char* name, IInterface* object);
    IContext* createSubcontext(const char* name);
    int destroySubcontext(const char* name);
    IInterface* lookup(const char* name);
    int rename(const char* oldName, const char* newName);
    int unbind(const char* name);
    IIterator* list(const char* name);

    static const Guid& interfaceID()
    {
        return IID_Context;
    }
};

class Iterator : public IIterator
{
    IMonitor*   monitor;
    Ref         ref;
    Binding*    binding;

public:
    Iterator(Binding* binding);
    ~Iterator();

    bool queryInterface(const Guid& riid, void** objectPtr);
    unsigned int addRef(void);
    unsigned int release(void);

    bool hasNext(void);
    IInterface* next();
    int remove(void);
};

#endif  // NINTENDO_ES_CONTEXT_H_INCLUDED
