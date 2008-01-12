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

#include <es.h>
#include <es/handle.h>
#include <es/ref.h>
#include <es/exception.h>
#include <es/base/IProcess.h>

ICurrentProcess* System();

class Test : public IInterface
{
    Ref ref;

public:
    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IInterface*>(this);
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

Test test;

int main(int argc, char* argv[])
{
    try
    {
        esReport("Hello, server.\n");
    }
    catch (Exception& error)
    {
        int errorCode = error.getResult();
        esReport("errorCode: %d\n", errorCode);
    }

    Handle<IStream> output(System()->getOut());
    Handle<IBinding> binding(output);
    binding->setObject(&test);
    binding->setObject(0);
}
