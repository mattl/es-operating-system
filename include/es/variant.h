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

#include <es/exception.h>

class VariantException : public Exception
{
public:
    virtual int getResult() const
    {
        return 0; // XXX: any error code ?
    }
};

class Variant
{
public:
    typedef int VariantType;
    static const VariantType VTypeOctet = 1;
    static const VariantType VTypeChar = 2;
    static const VariantType VTypeWChar = 3;
    static const VariantType VTypeString = 4;
    static const VariantType VTypeWString = 5;
    static const VariantType VTypeShort = 6;
    static const VariantType VTypeLong = 7;
    static const VariantType VTypeLongLong = 8;
    static const VariantType VTypeBool = 9;
    static const VariantType VTypeFloat = 10;
    static const VariantType VTypeDouble = 11;
    static const VariantType VTypeLongDouble = 12;
    static const VariantType VTypeObject = 13;

    Variant() :
        type(VTypeOctet),
        octetValue(0)
        {
        }

    Variant(unsigned char value) :
        type(VTypeOctet),
        octetValue(value)
        {
        }

    Variant(char value) :
        type(VTypeChar),
        charValue(value)
        {
        }

    Variant(wchar_t value) :
        type(VTypeWChar),
        wcharValue(value)
        {
        }

    Variant(char* value) :
        type(VTypeString),
        stringValue(value)
        {
        }

    Variant(wchar_t* value) :
        type(VTypeWString),
        wstringValue(value)
        {
        }

    Variant(short value) :
        type(VTypeShort),
        shortValue(value)
        {
        }

    Variant(int value) :
        type(VTypeLong),
        longValue(value)
        {
        }

    Variant(long long value) :
        type(VTypeLongLong),
        longLongValue(value)
        {
        }

    Variant(bool value) :
        type(VTypeBool),
        boolValue(value)
        {
        }

    Variant(float value) :
        type(VTypeFloat),
        floatValue(value)
        {
        }

    Variant(double value) :
        type(VTypeDouble),
        doubleValue(value)
        {
        }

    Variant(long double value) :
        type(VTypeLongDouble),
        longDoubleValue(value)
        {
        }

    Variant(void* value) :
        type(VTypeObject),
        objectValue(value)
        {
        }

    VariantType getType() const
    {
        return type;
    }

    operator unsigned char()
    {
        if (type != VTypeOctet)
        {
            throw VariantException();
        }
        return octetValue;
    }

    operator char()
    {
        if (type != VTypeChar)
        {
            throw VariantException();
        }
        return charValue;
    }

    operator wchar_t()
    {
        if (type != VTypeWChar)
        {
            throw VariantException();
        }
        return wcharValue;
    }

    operator char*()
    {
        if (type != VTypeString)
        {
            throw VariantException();
        }
        return stringValue;
    }

    operator wchar_t*()
    {
        if (type != VTypeWString)
        {
            throw VariantException();
        }
        return wstringValue;
    }

    operator short()
    {
        if (type != VTypeShort)
        {
            throw VariantException();
        }
        return shortValue;
    }

    operator int()
    {
        if (type != VTypeLong)
        {
            throw VariantException();
        }
        return longValue;
    }

    operator long long()
    {
        if (type != VTypeLongLong)
        {
            throw VariantException();
        }
        return longLongValue;
    }

    operator bool()
    {
        if (type != VTypeBool)
        {
            throw VariantException();
        }
        return boolValue;
    }

    operator float()
    {
        if (type != VTypeFloat)
        {
            throw VariantException();
        }
        return floatValue;
    }

    operator double()
    {
        if (type != VTypeDouble)
        {
            throw VariantException();
        }
        return doubleValue;
    }

    operator long double()
    {
        if (type != VTypeLongDouble)
        {
            throw VariantException();
        }
        return longDoubleValue;
    }

    operator void*()
    {
        if (type != VTypeObject)
        {
            throw VariantException();
        }
        return objectValue;
    }

private:
    union {
        unsigned char octetValue;
        char charValue;
        wchar_t wcharValue;
        char* stringValue;
        wchar_t* wstringValue;
        short shortValue;
        int longValue;
        long long longLongValue;
        bool boolValue;
        float floatValue;
        double doubleValue;
        long double longDoubleValue;
        void* objectValue;
    };
    VariantType type;
};

#endif // GOOGLE_ES_VARIANT_H_INCLUDED
