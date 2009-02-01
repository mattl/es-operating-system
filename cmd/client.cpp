/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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
#include <es/ref.h>
#include <es/exception.h>
#include <es/base/IProcess.h>



es::CurrentProcess* System();

class Test : public es::Interface
{
    Ref ref;

public:
    void* queryInterface(const char* riid)
    {
        void* objectPtr;
        if (strcmp(riid, es::Interface::iid()) == 0)
        {
            objectPtr = static_cast<es::Interface*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<es::Interface*>(objectPtr)->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

Test test;

int main(int argc, char* argv[])
{
    System()->trace(false);

    try
    {
        esReport("Hello, server.\n");
    }
    catch (Exception& error)
    {
        int errorCode = error.getResult();
        esReport("errorCode: %d\n", errorCode);
    }

    Handle<es::Stream> output(System()->getOutput());
    Handle<es::Binding> binding(output);
    binding->setObject(&test);
    binding->setObject(0);
}
