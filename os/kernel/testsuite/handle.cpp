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

#include <es.h>
#include <es/handle.h>
#include <es/base/IAlarm.h>
#include <es/naming/IContext.h>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

Handle<es::Context> classStore;

class Impl : public es::Interface
{
public:
    void* queryInterface(const char* riid)
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
    es::Interface* nameSpace;
    esInit(&nameSpace);

    esReport("%d\n", IsInterface<es::Interface>::Value);
    TEST(IsInterface<es::Interface>::Value);

    esReport("%d\n", IsInterface<es::Alarm>::Value);
    TEST(IsInterface<es::Alarm>::Value);

    esReport("%d\n", IsInterface<Impl>::Value);
    TEST(!IsInterface<Impl>::Value);

    Handle<es::Context> root(nameSpace);

    es::Interface* obj = root->lookup("class");
    classStore = obj;
    ASSERT(classStore);

    Handle<es::Context> a(root);
    Handle<es::Context> b(root);
    ASSERT(a == b);

    esReport("done.\n");
}
