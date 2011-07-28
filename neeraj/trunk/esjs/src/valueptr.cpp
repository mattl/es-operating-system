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
