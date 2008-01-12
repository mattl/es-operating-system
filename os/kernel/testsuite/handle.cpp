/*
 * Copyright (c) 2007
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
#include <es/handle.h>
#include <es/base/IClassStore.h>
#include <es/naming/IContext.h>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

Handle<IClassStore> classStore;

class Impl : public IInterface
{
public:
    void* queryInterface(const Guid& riid)
    {
        return NULL;
    }
    unsigned int addRef()
    {
        return 0;
    }
    unsigned int release()
    {
        return 0;
    }
};

int main()
{
    IInterface* nameSpace;
    esInit(&nameSpace);

    esReport("%d\n", IsInterface<IInterface>::Value);
    TEST(IsInterface<IInterface>::Value);

    esReport("%d\n", IsInterface<IClassStore>::Value);
    TEST(IsInterface<IClassStore>::Value);

    esReport("%d\n", IsInterface<Impl>::Value);
    TEST(!IsInterface<Impl>::Value);

    Handle<IContext> root(nameSpace);

    IInterface* obj = root->lookup("class");
    classStore = obj;
    ASSERT(classStore);

    Handle<IContext> a(root);
    Handle<IContext> b(root);
    ASSERT(a == b);

    esReport("done.\n");
}
