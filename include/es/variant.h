/*
 * Copyright 2008 Kenichi Ishibashi
 * Copyright 2008 Google Inc.
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

#ifndef GOOGLE_ES_VARIANT_H_INCLUDED
#define GOOGLE_ES_VARIANT_H_INCLUDED

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <es/exception.h>
#include <es/uuid.h>
#include <es/base/IInterface.h>

struct VariantBase
{
    union
    {
        bool            boolValue;
        uint8_t         octetValue;
        int16_t         shortValue;
        uint16_t        unsignedShortValue;
        int32_t         longValue;
        uint32_t        unsignedLongValue;
        int64_t         longLongValue;
        uint64_t        unsignedLongLongValue;
        float           floatValue;
        double          doubleValue;  // ES extension
        const char*     stringValue;  // DOMString in UTF-8
        es::IInterface* objectValue;

        const Guid*     guidValue;    // ES extension
    };
    int type;
};

// The any type for Web IDL
class Variant : private VariantBase
{
public:
    enum
    {
        TypeVoid,
        TypeBool,
        TypeOctet,
        TypeShort,
        TypeUnsignedShort,
        TypeLong,
        TypeUnsignedLong,
        TypeLongLong,
        TypeUnsignedLongLong,
        TypeFloat,
        TypeDouble,
        TypeString,
        TypeObject,
        TypeGuid,
        TypeSize,
        FlagVariant = 0x80000000
    };

    Variant()
    {
        longLongValue = 0;
        type = TypeVoid;
    }

    Variant(const VariantBase& v) :
        VariantBase(v)
    {
        type |= FlagVariant;
    }

    Variant(bool value)
    {
        boolValue = value;
        type = TypeBool;
    }

    Variant(uint8_t value)
    {
        octetValue = value;
        type = TypeOctet;
    }

    Variant(int16_t value)
    {
        shortValue = value;
        type = TypeShort;
    }

    Variant(uint16_t value)
    {
        unsignedShortValue = value;
        type = TypeUnsignedShort;
    }

    // Since int32_t can be either long or int, use int so as not to conflict with constructor for long
    Variant(int value)
    {
        longValue = value;
        type = TypeLong;
    }

    // Since uint32_t can be either long or int, use int so as not to conflict with constructor for unsigned long
    Variant(unsigned int value)
    {
        unsignedLongValue = value;
        type = TypeUnsignedLong;
    }

    // To support constants declared with intptr_t, constructor for long is necessary.
    Variant(long value)
    {
#if 2147483647L < LONG_MAX    
        longLongValue = value;
        type = TypeLongLong;
#else
        longValue = value;
        type = TypeLong;
#endif
    }

    // To support constants declared with uintptr_t, constructor for long is necessary.
    Variant(unsigned long value)
    {
#if 4294967295UL < ULONG_MAX
        unsignedLongValue = value;
        type = TypeUnsignedLong;
#else
        unsignedLongValue = value;
        type = TypeUnsignedLong;
#endif
    }

    // To support constants declared with LL, long long variant cannot be int64_t.
    Variant(long long value)
    {
        longLongValue = value;
        type = TypeLongLong;
    }

    // To support constants declared with LLu, long long variant cannot be uint64_t.
    Variant(unsigned long long value)
    {
        unsignedLongLongValue = value;
        type = TypeUnsignedLongLong;
    }

    Variant(float value)
    {
        floatValue = value;
        type = TypeFloat;
    }

    Variant(double value)
    {
        doubleValue = value;
        type = TypeDouble;
    }

    Variant(const char* value)
    {
        stringValue = value;
        type = TypeString;
    }

    Variant(es::IInterface* value)
    {
        objectValue = value;
        type = TypeObject;
    }

    Variant(const Guid* value)
    {
        guidValue = value;
        type = TypeGuid;
    }

    operator uint8_t() const
    {
        if (getType() != TypeOctet)
        {
            throw SystemException<EACCES>();
        }
        return octetValue;
    }

    operator int16_t() const
    {
        if (getType() != TypeShort)
        {
            throw SystemException<EACCES>();
        }
        return shortValue;
    }

    operator uint16_t() const
    {
        if (getType() != TypeUnsignedShort)
        {
            throw SystemException<EACCES>();
        }
        return unsignedShortValue;
    }

    operator int32_t() const
    {
        if (getType() != TypeLong)
        {
            throw SystemException<EACCES>();
        }
        return longValue;
    }

    operator uint32_t() const
    {
        if (getType() != TypeUnsignedLong)
        {
            throw SystemException<EACCES>();
        }
        return unsignedLongValue;
    }

    operator int64_t() const
    {
        if (getType() != TypeLongLong)
        {
            throw SystemException<EACCES>();
        }
        return longLongValue;
    }

    operator uint64_t() const
    {
        if (getType() != TypeUnsignedLongLong)
        {
            throw SystemException<EACCES>();
        }
        return unsignedLongLongValue;
    }

    operator bool() const
    {
        if (getType() != TypeBool)
        {
            throw SystemException<EACCES>();
        }
        return boolValue;
    }

    operator float() const
    {
        if (getType() != TypeFloat)
        {
            throw SystemException<EACCES>();
        }
        return floatValue;
    }

    operator double() const
    {
        if (getType() != TypeDouble)
        {
            throw SystemException<EACCES>();
        }
        return doubleValue;
    }

    operator const char*() const
    {
        if (getType() != TypeString)
        {
            throw SystemException<EACCES>();
        }
        return stringValue;
    }

    operator es::IInterface*() const
    {
        if (getType() != TypeObject)
        {
            throw SystemException<EACCES>();
        }
        return objectValue;
    }

    operator const Guid*() const
    {
        if (getType() != TypeGuid)
        {
            throw SystemException<EACCES>();
        }
        return guidValue;
    }

    int getType() const
    {
        return (type & ~FlagVariant);
    }

    void makeVariant()
    {
        type |= FlagVariant;
    }

    bool isVariant() const
    {
        return (type & FlagVariant) ? true : false;
    }
};

Variant apply(int argc, Variant* argv, Variant (*function)());
Variant apply(int argc, Variant* argv, bool (*function)());
Variant apply(int argc, Variant* argv, uint8_t (*function)());
Variant apply(int argc, Variant* argv, int16_t (*function)());
Variant apply(int argc, Variant* argv, uint16_t (*function)());
Variant apply(int argc, Variant* argv, int32_t (*function)());
Variant apply(int argc, Variant* argv, uint32_t (*function)());
Variant apply(int argc, Variant* argv, int64_t (*function)());
Variant apply(int argc, Variant* argv, uint64_t (*function)());
Variant apply(int argc, Variant* argv, float (*function)());
Variant apply(int argc, Variant* argv, double (*function)());
Variant apply(int argc, Variant* argv, const char* (*function)());
Variant apply(int argc, Variant* argv, es::IInterface* (*function)());

long long evaluate(const Variant& variant);

#endif // GOOGLE_ES_VARIANT_H_INCLUDED
