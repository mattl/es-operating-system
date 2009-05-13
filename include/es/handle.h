/*
 * Copyright 2008, 2009 Google Inc.
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

#ifndef NINTENDO_ES_HANDLE_H_INCLUDED
#define NINTENDO_ES_HANDLE_H_INCLUDED

#include <stddef.h>
#include <es/object.h>

/** Common base class for boolean type traits.
 */
template <bool C>
struct BooleanType
{
    static const bool Value = C;
};

/** Base class for type traits that are true.
 */
typedef BooleanType<true>   TrueType;

/** Base class for type traits that are false.
 */
typedef BooleanType<false>  FalseType;

template <typename T>
class IsInterfaceImp
{
    // sizeof(YesType) != sizeof(NoType)
    typedef char YesType;
    class NoType
    {
        char    padding[2];
    };

    // With a compiler that supports DR 337, e.g., GCC 3.4 and later, the
    // template type argument deduction fails when attempting to create
    // an array with an abstract class type.
    template<typename U>
    static NoType isAbstract(U (*array)[1]);
    template<typename U>
    static YesType isAbstract(...);

    static const bool IsAbstract = sizeof(isAbstract<T>(0)) == sizeof(YesType);

    // IsObject has to deal with ambiguous 'Object' base classes.
    // See http://groups.google.com/group/comp.lang.c++.moderated/msg/dd6c4e4d5160bd83
    struct C
    {
        operator Object const volatile *() const;
        operator T const volatile *();
    };

    static C getObject();

    template <typename U>
    static YesType isObject(T const volatile *, U);
    static NoType isObject(Object const volatile *, int);

    static const bool IsObject = sizeof(isObject(getObject(), 0)) == sizeof(YesType);

public:
    static const bool Value = IsAbstract && IsObject;
};

template<>
class IsInterfaceImp<Object>
{
public:
    static const bool Value = true;
};

/** If <code>T</code> is an abstruct interface that inherits
 * <code>Object</code> then <code>IsInterface</code> inherits from
 * <code>TrueType</code>, otherwise inherits from <code>FalseType</code>.
 */
template <typename T>
struct IsInterface : BooleanType<IsInterfaceImp<T>::Value>
{
    /** <code>true</code> if <code>T</code> is an abstruct interface
     * that inherits from <code>Object</code>, otherwise
     * <code>false</code>.
     */
    static const bool Value = IsInterfaceImp<T>::Value;
};

/** This template class provides a smart pointer for managing
 * interface pointers.
 * <code>Handle</code> has a template parameter, <code>I</code>,
 * which specifies the type of interface to be used.
 * <code>I</code> must be derived from <code>Object</code> or
 * <code>Object</code>.
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
 *         Object* unknown = iterator->next();
 *         es::Binding* binding;
 *         unknown->queryInterface(es::Binding::iid(),
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
 * Note the interface UUID must be retrieved as a static member
 * <code>iid</code> so that the constructor can invoke
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
        Object* queryInterface(const char* riid);

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

    /** Casts unknown to I*, where I is an abstract interface.
     */
    template<class J>
    static I* cast(J* unknown, TrueType)
    {
        if (!unknown)
        {
            return 0;
        }
        else
        {
            void* to = unknown->queryInterface(I::iid());
            return static_cast<I*>(to);
        }
    }

    /** Casts unknown to I*, where I is an interface implementation.
     */
    template<class J>
    static I* cast(J* unknown, FalseType)
    {
        I* to = dynamic_cast<I*>(unknown);
        if (to)
        {
            to->addRef();
        }
        return to;
    }

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
        object = cast(unknown, IsInterface<J>());
        if (object && !addRef)
        {
            unknown->release();
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
        Object* unknown = comptr.get();
        object = cast(unknown, IsInterface<J>());
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
