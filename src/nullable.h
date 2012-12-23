/*
 * Copyright 2011, 2012 Esrille Inc.
 * Copyright 2008-2010 Google Inc.
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

#ifndef ES_NULLABLE_H_INCLUDED
#define ES_NULLABLE_H_INCLUDED

#include <string>
#include <typeinfo>
#include <type_traits>

class Any;
class Object;

template <typename T>
class Nullable
{
    T    value_;
    bool hasValue_;

    template <typename U>
    Nullable(const Nullable<U>&);

public:
    bool hasValue() const
    {
        return hasValue_;
    }

    T value() const
    {
        if (!hasValue_)
        {
            // TODO: Raise an exception
        }
        return value_;
    }

    operator std::u16string() const;

    Nullable() :
        hasValue_(false)
    {
    }

    Nullable(const T& value) :
        value_(value),
        hasValue_(true)
    {
    }

    Nullable(const Nullable<T>& nullable) :
        value_(nullable.value_),
        hasValue_(nullable.hasValue_)
    {
    }

    bool operator==(const Nullable<T>& other) const
    {
        if (!hasValue_)
            return !other.hasValue_;
        if (!other.hasValue_)
            return false;
        return value_ == other.value_;
    }
    bool operator!=(const Nullable<T>& other) const
    {
        return !(*this == other);
    }

    template<typename U>
    Nullable(U* str);

    Nullable(const Any& any);
};

template<>
template<>
inline Nullable<std::string>::Nullable(const char* str)
{
    if (!str)
        hasValue_ = false;
    else {
        hasValue_ = true;
        value_ = str;
    }
}

template<>
template<>
inline Nullable<std::u16string>::Nullable(const char16_t* str)
{
    if (!str)
        hasValue_ = false;
    else {
        hasValue_ = true;
        value_ = str;
    }
}

// Note maybe it is a bit arguable whether we should have this
// conversion operator. The primary use case of this is to call
// ElementImp::getAttribute from Element's subclasses with a
// specific qualified name.
template<>
inline Nullable<std::u16string>::operator std::u16string() const
{
    if (hasValue_)
        return value_;
    else
        return u"";
}

#endif  // ES_NULLABLE_H_INCLUDED
