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

#include <stdio.h>
#include <es.h>
#include <es/handle.h>
#include <es/ref.h>
#include <es/base/ICallback.h>

using namespace es;

class A : public ICallback
{
    Ref ref;

public:
    ~A()
    {
        printf("done.\n");
    }

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == IInterface::iid())
        {
            objectPtr = static_cast<ICallback*>(this);
        }
        else if (riid == ICallback::iid())
        {
            objectPtr = static_cast<ICallback*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
        return objectPtr;
    }

    unsigned int addRef(void)
    {
        return ref.addRef();
    }

    unsigned int release(void)
    {
        unsigned int count = ref.release();
        printf("count: %d\n", count);
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }

    int invoke(int result)
    {
        return result;
    }
};

class B
{
    Ref ref;

public:
    unsigned int addRef(void)
    {
        return ref.addRef();
    }

    unsigned int release(void)
    {
        unsigned int count = ref.release();
        printf("count: %d\n", count);
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

class C : public B
{
public:
    void test()
    {
    }
};

int main()
{
    A a;
    C c;

    printf("sizeof(IInterface) :%d\n", sizeof(IInterface));
    printf("sizeof(A) :%d\n", sizeof(A));

    printf("isInterface(IInterface) :%d\n", IsInterface<IInterface>::Value);
    printf("isInterface(A) :%d\n", IsInterface<A>::Value);

    Handle<ICallback> hcb(&a, true);
    Handle<B> hc(&c, true);

    Handle<A> ha(hcb);
}
