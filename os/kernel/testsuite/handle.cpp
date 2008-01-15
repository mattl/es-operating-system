/*
 * Copyright 2008 Google Inc.
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
