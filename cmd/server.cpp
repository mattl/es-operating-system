/*
 * Copyright (c) 2006
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
#include <string.h>
#include <es.h>
#include <es/clsid.h>
#include <es/exception.h>
#include <es/handle.h>
#include <es/ref.h>
#include <es/base/IClassStore.h>
#include <es/base/IProcess.h>
#include <es/naming/IBinding.h>

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

ICurrentProcess* System();
Handle<IClassStore> classStore;

bool createInstance(const Guid& rclsid, const Guid& riid, void** objectPtr)
{
    return classStore->createInstance(rclsid, riid, objectPtr);
}

class Stream : public IStream, public IBinding
{
    Ref         ref;
    IInterface* object;

public:
    Stream() :
        object(0)
    {
    }

    ~Stream()
    {
        if (object)
        {
            object->release();
        }
    }

    long long getPosition()
    {
        return 0;
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
        return 0;
    }

    int write(const void* src, int count)
    {
        if (count == 1)
        {
            char c = *(const char*) src;
            if (c == ',')
            {
                esThrow(EBUSY);
            }
        }
        return esReport("%.*s", count, src);
    }

    int write(const void* src, int count, long long offset)
    {
        return esReport("%.*s", count, src);
    }

    void flush()
    {
    }

    IInterface* getObject()
    {
        return object;
    }

    int setObject(IInterface* element)
    {
        esReport("Stream::setObject(%p)\n", element);
        if (element)
        {
            element->addRef();
        }
        if (object)
        {
            object->release();
        }
        object = element;
    }

    int getName(char* name, unsigned int len)
    {
        unsigned count = (len < 7) ? len : 7;
        strncpy(name, "Stream", count);
        return count;
    }

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IStream*>(this);
        }
        else if (riid == IID_IStream)
        {
            *objectPtr = static_cast<IStream*>(this);
        }
        else if (riid == IID_IBinding)
        {
            *objectPtr = static_cast<IBinding*>(this);
            esReport("Stream::queryInterface: %p@%p\n", *objectPtr, objectPtr);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
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

int main(int argc, char* argv[])
{
    esReport("Hello, world.\n");

    Handle<IContext> nameSpace = System()->getRoot();
    classStore = nameSpace->lookup("class");
    TEST(classStore);

    // Create a client process.
    Handle<IProcess> client;
    bool result = createInstance(CLSID_Process, IID_IProcess,
                                 reinterpret_cast<void**>(&client));
    TEST(result);

    // Set the standard output to the client process.
    Stream stream;
    stream.write("Test stream.\n", 13);
    client->setOut(&stream);

    // Start the client process.
    Handle<IFile> file = nameSpace->lookup("file/client.elf");
    TEST(file);
    client->start(file);

    // Wait for the client to exit
    client->wait();
}
