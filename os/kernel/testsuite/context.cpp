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

#include <es.h>
#include "context.h"

#define NumOf(x)        (sizeof(x)/sizeof((x)[0]))
#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

class Foo : public IInterface
{
    int ref;

public:
    Foo() : ref(1)
    {
    }

    ~Foo()
    {
    }

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IInterface*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
    }

    unsigned int addRef(void)
    {
        return ++ref;
    }

    unsigned int release(void)
    {
        if (--ref == 0)
        {
            delete this;
        }
        return ref;
    }
};

static bool IsNameInList(const char** namelist, int num, const char* name)
{
    while (0 < num)
    {
        if (strcmp(namelist[num-1], name) == 0)
        {
            return true;
        }
        --num;
    }
    return false;
}

static int test_without_smart_pointer(IContext* context)
{
    IIterator* iterator;
    char name[64];
    IBinding* binding;
    IInterface* unknown;
    Foo foo;

    binding = context->bind("a", &foo);
    binding->release();
    binding = context->bind("b", &foo);
    binding->release();

    IContext* sub = context->createSubcontext("sub");
    sub->release();
    binding = context->bind("sub/d", &foo);
    binding->release();

    unknown = context->lookup("sub/d");
    TEST(unknown == static_cast<IInterface*>(&foo));
    unknown->release();

    const char* namelist[] =
    {
        "a",
        "b",
        "sub"
    };
    int numName = 0;

    iterator = context->list("");
    while (iterator->hasNext())
    {
        unknown = iterator->next();
        unknown->queryInterface(IID_IBinding, reinterpret_cast<void**>(&binding));
        unknown->release();
        binding->getName(name, sizeof name);
#ifdef VERBOSE
        esReport("%s\n", name);
#endif // VERBOSE
        binding->release();

        TEST(IsNameInList(namelist, NumOf(namelist), name));
        ++numName;
    }
    iterator->release();

    TEST(numName == NumOf(namelist));

    // check destroySubcontext(), rename() and unbind().
    sub = context->createSubcontext("sub2");
    sub->release();
    context->destroySubcontext("sub2");

    context->rename("a", "c");
    context->unbind("c");

    const char* namelist2[] =
    {
        "b",
        "sub"
    };

    numName = 0;
    iterator = context->list("");
    while (iterator->hasNext())
    {
        unknown = iterator->next();
        unknown->queryInterface(IID_IBinding, reinterpret_cast<void**>(&binding));
        unknown->release();
        binding->getName(name, sizeof name);
#ifdef VERBOSE
        esReport("%s\n", name);
#endif // VERBOSE
        binding->release();
        iterator->remove();

        TEST(IsNameInList(namelist, NumOf(namelist), name));
        ++numName;
    }
    iterator->release();

    TEST(numName == NumOf(namelist2));

    return 0;
}

static int test_with_smart_pointer(IContext* _context)
{

    char name[64];
    Foo foo;
    Handle<IBinding> binding;

    Handle<IContext> context(_context, true);
    binding = context->bind("a", &foo);
    binding = context->bind("b", &foo);

    Handle<IContext> sub = context->createSubcontext("sub");
    binding = context->bind("sub/d", &foo);

    const char* namelist[] =
    {
        "a",
        "b",
        "sub"
    };

    int numName = 0;
    Handle<IIterator> iterator = context->list("");
    while (iterator->hasNext())
    {
        binding = iterator->next();
        binding->getName(name, sizeof name);

        TEST(IsNameInList(namelist, NumOf(namelist), name));
        ++numName;
    }

    TEST(numName == NumOf(namelist));
    iterator = 0;

    // check destroySubcontext(), rename() and unbind().
    Handle<IContext> sub2 = context->createSubcontext("sub2");
    context->destroySubcontext("sub2");

    context->rename("a", "c");
    context->unbind("c");

    const char* namelist2[] =
    {
        "b",
        "sub"
    };

    numName = 0;
    iterator = context->list("");
    while (iterator->hasNext())
    {
        binding = iterator->next();
        binding->getName(name, sizeof name);
#ifdef VERBOSE
        esReport("%s\n", name);
#endif // VERBOSE
        iterator->remove();

        TEST(IsNameInList(namelist2, NumOf(namelist2), name));
        ++numName;
    }
    TEST(numName == NumOf(namelist2));

    return 0;
}

int main()
{
    IInterface* root = NULL;
    esInit(&root);
    IContext* context;

    int ret;
    context = new Context;
    ret = test_without_smart_pointer(context);
    context->release();
    if (ret < 0)
    {
        return 1;
    }

    context = new Context;
    ret = test_with_smart_pointer(context);
    context->release();
    if (ret < 0)
    {
        return 1;
    }

    esReport("done.\n");
    return 0;
}
