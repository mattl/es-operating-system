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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/ring.h>
#include <es/ref.h>
#include <es/synchronized.h>
#include <es/usage.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <es/base/IInterfaceStore.h>
#include <es/naming/IContext.h>

using namespace es;

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

ICurrentProcess* System();

class TestServer : public IStream
{
public:
    Ref     ref;

    TestServer()
    {
    }

    ~TestServer()
    {
    }

    //
    // IStream
    //
    long long getPosition()
    {
    }

    void setPosition(long long pos)
    {
    }

    long long getSize()
    {
        return 0;
    }

    void setSize(long long size)
    {
    }

    int read(void* dst, int count)
    {
        return 0;
    }

    int read(void* dst, int count, long long offset)
    {
        return read(dst, count);
    }

    int write(const void* src, int count)
    {
        esReport("%s : %d\n", src, count);
        return count;
    }

    int write(const void* src, int count, long long offset)
    {
        return write(src, count); // [check] use offset?
    }

    void flush()
    {
    }

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == IStream::iid())
        {
            objectPtr = static_cast<IStream*>(this);
        }
        else if (riid == IInterface::iid())
        {
            objectPtr = static_cast<IStream*>(this);
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
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

int main(int argc, char* argv[])
{
    esReport("This is the upcall test server process.\n");

    // System()->trace(true);

    Handle<IContext> nameSpace = System()->getRoot();
    Handle<ICurrentThread> currentThread = System()->currentThread();

    // create server
    TestServer* server = new TestServer;

    // register this console.
    Handle<IContext> device = nameSpace->lookup("device");
    ASSERT(device);
    IBinding* ret = device->bind("testServer", static_cast<IStream*>(server));
    ASSERT(ret);
    ret->release();

    while (1 < server->ref)
    {
        currentThread->sleep(10000000LL);
    }

    server->release();

    System()->trace(false);
}
