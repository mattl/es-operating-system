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
