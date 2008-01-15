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

#ifndef NINTENDO_ESJS_SMARTPTR_H_INCLUDED
#define NINTENDO_ESJS_SMARTPTR_H_INCLUDED

template<class X>
class SmartPtr
{
    // cf. Colvin-Gibbons trick
    template<class Y>
    struct SmartPtrRef
    {
        Y* ptr;

        SmartPtrRef(Y* p) :
            ptr(p)
        {
        }
    };

    X* ptr;

public:

    SmartPtr(X* p = 0) throw() :
        ptr(p)
    {
    }

    template<class Y>
    SmartPtr(Y* p = 0) throw() :
        ptr(p)
    {
    }

    SmartPtr(const SmartPtr& p) throw() :
        ptr(p.ptr)
    {
        if (ptr)
        {
            ptr->addRef();
        }
    }

    template<class Y>
    SmartPtr(const SmartPtr<Y>& p) throw() :
        ptr(p.ptr)
    {
        if (ptr)
        {
            ptr->addRef();
        }
    }

    SmartPtr& operator=(X* p) throw()
    {
        if (ptr != p)
        {
            if (ptr)
            {
                ptr->release();
            }
            ptr = p;
        }
    }

    template<class Y>
    SmartPtr& operator=(Y* p) throw()
    {
        if (ptr != p)
        {
            if (ptr)
            {
                ptr->release();
            }
            ptr = p;
        }
    }

    SmartPtr& operator=(SmartPtr& p) throw()
    {
        if (ptr != p.ptr)
        {
            if (p.ptr)
            {
                p.ptr->addRef();
            }
            if (ptr)
            {
                ptr->release();
            }
            ptr = p.ptr;
        }
    }

    template<class Y>
    SmartPtr& operator=(SmartPtr<Y>& p) throw()
    {
        if (ptr != p.ptr)
        {
            if (p.ptr)
            {
                p.ptr->addRef();
            }
            if (ptr)
            {
                ptr->release();
            }
            ptr = p.ptr;
        }
    }

    ~SmartPtr() throw()
    {
        if (ptr)
        {
            ptr->release();
        }
    }


    X& operator*() const throw()
    {
        return *ptr;
    }

    X* operator->() const throw()
    {
        return ptr;
    }

    operator X*() const
    {
        return ptr;
    }

    SmartPtr(SmartPtrRef<X> r) throw() :
        ptr(r.ptr)
    {
        if (ptr)
        {
            ptr->addRef();
        }
    }

    SmartPtr& operator=(SmartPtrRef<X> r) throw()
    {
        if (ptr != r.ptr)
        {
            if (r.ptr)
            {
                r.ptr->addRef();
            }
            if (ptr)
            {
                ptr->release();
            }
            ptr = r.ptr;
        }
    }

    template<class Y>
    operator SmartPtrRef<Y>() throw()
    {
        return SmartPtrRef<Y>(ptr);
    }

    template<class Y>
    operator SmartPtr<Y>() throw()
    {
        return SmartPtr<Y>(ptr);
    }
};

#endif  // NINTENDO_ESJS_SMARTPTR_H_INCLUDED
