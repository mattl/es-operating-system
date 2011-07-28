/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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
#include <es.h>
#include <es/handle.h>
#include <es/ref.h>
#include <es/base/ICallback.h>

using namespace es;

class A : public Callback
{
    Ref ref;

public:
    ~A()
    {
        printf("done.\n");
    }

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<Callback*>(this);
        }
        else if (strcmp(riid, Callback::iid()) == 0)
        {
            objectPtr = static_cast<Callback*>(this);
        }
        else
        {
            return NULL;
        }
        objectPtr->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
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
    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
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

    printf("sizeof(Object) :%d\n", sizeof(Object));
    printf("sizeof(A) :%d\n", sizeof(A));

    printf("isInterface(Object) :%d\n", IsInterface<Object>::Value);
    printf("isInterface(A) :%d\n", IsInterface<A>::Value);

    Handle<Callback> hcb(&a, true);
    Handle<B> hc(&c, true);

    Handle<A> ha(hcb);
}
