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

#include <stdio.h>
#include <es/ref.h>
#include "smartptr.h"

class A
{
    Ref ref;
public:
    virtual ~A()
    {
        printf("%s\n", __func__);
    }

    virtual unsigned int addRef()
    {
        printf("%s\n", __func__);
        return ref.addRef();
    }

    virtual unsigned int release()
    {
        printf("%s\n", __func__);
        if (ref.release() == 0)
        {
            delete this;
        }
    }
};


class B : public A
{
public:
    virtual ~B()
    {
        printf("%s\n", __func__);
    }
};

SmartPtr<A> test1()
{
    SmartPtr<A> x = new B;

    return new B;
}

SmartPtr<A> test2()
{
    SmartPtr<A> x = test1();
    SmartPtr<A> y = x;
    SmartPtr<A> z(y);
    return y;
}

int main()
{
    test2();

    printf("done.\n");
}
