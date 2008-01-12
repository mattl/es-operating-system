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

#ifndef NINTENDO_ES_HANDLE_H_INCLUDED
#define NINTENDO_ES_HANDLE_H_INCLUDED

#include <stddef.h>
#include <es/base/IInterface.h>

/** This template class provides a smart pointer for managing
 * interface pointers.
 * <code>Handle</code> has a template parameter, <code>I</code>,
 * which specifies the type of interface to be used.
 * <code>I</code> must be derived from <code>IInterface</code> or
 * <code>IInterface</code>.
 * <p>
 * <code>Handle</code> is smart in that it manages
 * <code>addRef</code>, <code>release</code> and <code>queryInterface</code>
 * implicitly. The following code lists up names in a naming context
 * using raw interface pointers:
 *
 * <p>
 * <table border=1 cellspacing=0 cellpadding=12 width="100%"><tr><td>
 * <table width="100%" bgcolor="#eeeeee">
 * <caption align="bottom">Example using raw interface pointers</caption>
 * <tr><td><pre>
 * {
 *     IIterator* iterator = context->list("");
 *     while (iterator->hasNext())
 *     {
 *         IInterface* unknown = iterator->next();
 *         IBinding* binding;
 *         unknown->queryInterface(IID_IBinding,
 *                                 reinterpret_cast&lt;void**&gt;(&binding));
 *         unknown->release();
 *         binding->getName(name, sizeof name);
 *         esReport("%s\n", name);
 *         binding->release();
 *     }
 *     iterator->release();
 * }
 * </pre></td></table>
 * </td></table>
 * <p>
 * The equivalent operation is written more concisely using
 * <code>Handle</code>s:
 * <p>
 * <table border=1 cellspacing=0 cellpadding=12 width="100%"><tr><td>
 * <table width="100%" bgcolor="#eeeeee">
 * <caption align="bottom">Example using <code>Handle</code>s</caption>
 * <tr><td><pre>
 * {
 *     Handle&lt;IIterator&gt; iterator = context->list("");
 *     while (iterator->hasNext())
 *     {
 *         Handle&lt;IBinding&gt; binding(iterator->next());
 *         binding->getName(name, sizeof name);
 *         esReport("%s\n", name);
 *     }
 * }
 * </pre></td></table>
 * </td></table>
 * <p>
 * Methods <code>addRef</code>, <code>release</code>, and
 * <code>queryInterface</code> cannot be directly accessed from <code>Handle</code>s
 * to help prevent leaks caused by misuse.
 * A <code>Handle</code> constructor that takes a <code>Handle</code>
 * argument of another interface type automatically invokes
 * <code>queryInterface</code> for the interface to be used.
 * Note the interface UUID must be retrieved using <code>interfaceID</code>
 * static member function so that the constructor can invoke
 * <code>queryInterface</code> for the appropriate interface type.
 */
template<class I>
class Handle
{
    I* object;  // interface pointer

    /** The <code>Protected</code> class makes it a compile-time
     * error to call <code>addRef</code>, <code>release</code>, and
     * <code>queryInterface</code> on a <code>Handle</code>.
     */
    class Protected : public I
    {
    private:
        // Do not replace the following three declarations by
        // using-declarations like:
        //
        //  using I::addRef;
        //  using I::release;
        //  using I::queryInterface;
        //
        // as C++ ignores names introduced by using-declarations for
        // a virtual function.
        unsigned int addRef();
        unsigned int release();
        bool queryInterface(const Guid& riid, void** objectPtr);

        void operator delete(void*, size_t);
        Protected& operator=(const I&);

        Protected(); // GCC 3.x attempts to construct this constructor
                     // even though it is never called. Thus if class
                     // I does not have a default constructor without
                     // an argument, GCC 3.x generates a compile error
                     // if this constructor is not declared. Note that
                     // the definition of this constructor is not
                     // necessary.
    };

public:
    /** Default constructor
     */
    Handle() : object(0)
    {
        return;
    }

    /** Constructor from a raw pointer of the different interface type.
     * <code>queryInterface</code> is implicitly called.
     * <code>addRef</code> is not called by default.
     */
    template<class J>
    Handle(J* unknown, bool addRef = false)
    {
        if (!unknown)
        {
            object = 0;
        }
        else
        {
            void* newPtr;
            if (!unknown->queryInterface(I::interfaceID(), &newPtr))
            {
                newPtr = 0;
            }
            else if (!addRef)
            {
                unknown->release();
            }
            object = static_cast<I*>(newPtr);
        }
    }

    /** Constructor from a raw pointer of the same interface type.
     * <code>addRef</code> is not called by default.
     */
    Handle(I* object, bool addRef = false) : object(object)
    {
        if (object && addRef)
        {
            object->addRef();
        }
    }

    /** Constructor from a smart pointer of the different interface type.
     * <code>queryInterface</code> is implicitly called.
     */
    template<class J>
    Handle(const Handle<J>& comptr)
    {
        IInterface* unknown = comptr.get();
        if (!unknown)
        {
            object = 0;
        }
        else
        {
            void* newPtr;
            if (!unknown->queryInterface(I::interfaceID(), &newPtr))
            {
                newPtr = 0;
            }
            object = static_cast<I*>(newPtr);
        }
    }

    /** Copy-constructor.
     */
    Handle(const Handle<I>& comptr) : object(comptr.object)
    {
        if (object)
        {
            object->addRef();
        }
    }

    /** Default destructor automatically invokes <code>release</code>.
     */
    ~Handle()
    {
        if (object)
        {
            object->release();
        }
    }

    /** Copy assignment operator.
     */
    Handle& operator=(const Handle& other)
    {
        // if (other.object)
        // {
        //     other.object->addRef();
        // }
        // I* temp(object);
        // object = other.object;
        // if (temp)
        // {
        //     temp->release();
        // }
        // return *this;
        //
        //       V
        //
        Handle temp(other, true);
        swap(temp);
        return *this;
    }

    /** Assignment operator from a raw pointer of the same interface type.
     */
    Handle& operator=(I* other)
    {
        Handle temp(other);
        swap(temp);
        return *this;
    }

    /** Assignment operator from a raw pointer of the different interface type.
     */
    template<class J>
    Handle& operator=(J* other)
    {
        Handle temp(other);
        swap(temp);
        return *this;
    }

    /** This method is called from assignment operators.
     */
    void swap(Handle& other) throw()
    {
        I* temp(object);
        object = other.object;
        other.object = temp;
    }

    /** This method is called from the -> operator and the conversion function
     * to a raw pointer type.
     */
    Protected* get() const
    {
        return reinterpret_cast<Protected*>(object);
    }

    /** Class member access function.
     */
    Protected* operator->() const
    {
        return get();
    }

    /** Conversion function to a protected raw pointer type. This
     * operator makes <code>Handle</code> act like its underlying
     * raw pointer type.
     */
    operator Protected*() const
    {
        return get();
    }
};

template<class I, class J>
inline bool operator==(const Handle<I>& a, const Handle<J>& b)
{
    return static_cast<const void*>(a.get()) == static_cast<const void*>(b.get());
}

template<class I, class J>
inline bool operator!=(const Handle<I>& a, const Handle<J>& b)
{
    return static_cast<const void*>(a.get()) != static_cast<const void*>(b.get());
}

template<class I>
inline bool operator==(const Handle<I>& a, I* b)
{
    return static_cast<const void*>(a.get()) == static_cast<const void*>(b);
}

template<class I>
inline bool operator!=(const Handle<I>& a, I* b)
{
    return static_cast<const void*>(a.get()) != static_cast<const void*>(b);
}

template<class I>
inline bool operator==(I* a, const Handle<I>& b)
{
    return static_cast<const void*>(a) == static_cast<const void*>(b.get());
}

template<class I>
inline bool operator!=(I* a, const Handle<I>& b)
{
    return static_cast<const void*>(a) != static_cast<const void*>(b.get());
}

#endif  // #ifndef NINTENDO_ES_HANDLE_H_INCLUDED
