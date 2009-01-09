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

#ifndef NINTENDO_ES_BASE_ICLASS_FACTORY_TEMPLATE_H_INCLUDED
#define NINTENDO_ES_BASE_ICLASS_FACTORY_TEMPLATE_H_INCLUDED

#include <new>
#include <es.h>
#include <errno.h>
#include <es/exception.h>
#include <es/ref.h>
#include <es/base/IClassFactory.h>

/**
 * This class implements the <code>IClassFactory</code> interface.
 * @param C the class of objects to be constructed from the <code>ClassFactory</code> class.
 *        C must have a default constructor.
 */
template<class C>
class ClassFactory : public es::IClassFactory
{
    Ref ref;

public:
    void* createInstance(const char* riid)
    {
        void* objectPtr = 0;
        C* instance = new(std::nothrow) C;
        if (!instance)
        {
            throw SystemException<ENOMEM>();
        }
        objectPtr = instance->queryInterface(riid);
        instance->release();
        return objectPtr;
    }

    void* queryInterface(const char* riid)
    {
        void* objectPtr;
        if (riid == es::IClassFactory::iid())
        {
            objectPtr = static_cast<es::IClassFactory*>(this);
        }
        else if (riid == es::IInterface::iid())
        {
            objectPtr = static_cast<es::IClassFactory*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<es::IInterface*>(objectPtr)->addRef();
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
        }
        return count;
    }
};

#endif  // #ifndef NINTENDO_ES_BASE_ICLASS_FACTORY_TEMPLATE_H_INCLUDED
