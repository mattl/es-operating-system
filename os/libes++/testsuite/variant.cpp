/*
 * Copyright 2011 Esrille Inc.
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2008 Kenichi Ishibashi
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

#include <assert.h>
#include <cmath>
#include <cstring>
#include <locale.h>
#include <stdio.h>
#include <wchar.h>
#include <algorithm>
#include <es/any.h>

class VariantHolder
{
    static const int STRING_CHAR_MAX = 128;

    Any var;
    char* string;
    int length;

public:
    VariantHolder()
    {
    }

    ~VariantHolder()
    {
        if (var.getType() == Any::TypeString && string)
        {
            delete string;
        }
    }

    Any getVar(void* value, int valueLength)
    {
        int type = var.getType();
        if (type == Any::TypeString)
        {
            strncpy(static_cast<char*>(value), string, valueLength);
            return static_cast<const char*>(value);
        }
        return var;
    }

    int setVar(Any value)
    {
        int type = value.getType();
        if (type == Any::TypeString)
        {
            const char* ptr = static_cast<const char*>(value);
            length = std::min(static_cast<int>(strlen(ptr)) + 1, STRING_CHAR_MAX);
            string = new char[length];
            strncpy(string, ptr, length);
            var = string;
        }
        else
        {
            var = value;
            length = 0;
        }
        return length;
    }
};

const int VariantHolder::STRING_CHAR_MAX;

Any returnVariant(int x, int y)
{
    return Any(x * y);
}

uint8_t returnOctet(int n1, int n2, int n3, int n4, int n5, Any v)
{
    printf("v: type = %d (%d)\n", v.getType(), static_cast<int32_t>(v));
    return 'c';
}

float returnFloat(bool x)
{
    return !x ? 10.0f : 20.0f;
}

long long returnLongLong(int a, double x0, int b, int c,
                         int d, int e, int f, int g, int h, int i,
                         double x1, double x2, double x3, double x4,
                         double x5, double x6, double x7, double x8, int j)
{
    printf("int: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", a, b, c, d, e, f, g, h, i, j);
    printf("double: %g, %g, %g, %g, %g, %g, %g, %g, %g\n", x0, x1, x2, x3, x4, x5, x6, x7, x8);
    return 200LL;
}

int raise()
{
    throw 1;
    return 0;
}

void* map(void* start, long long length, unsigned prot, unsigned flags, long long offset)
{
    printf("%s(%p, %lld, %x, %x, %lld)\n", __func__, start, length, prot, flags, offset);
    return 0;
}

int main()
{
    VariantHolder vh;
    char buf[128]; 
    Any param[19];
    Any value;

    printf("sizeof(Any) = %u, offsetof(AnyBase, type) = %u\n", sizeof(Any), offsetof(AnyBase, type));

    value = returnVariant(2, 3);
    assert(value.getType() == Any::TypeLong);
    assert(static_cast<int32_t>(value) == 6);
    printf("%d\n", evaluate(value));

    returnLongLong(1, 0.0, 2, 3, 4, 5, 6, 7, 8, 9,
                   1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 10);

    value = returnVariant(2, 3);
    printf("int32 value: %d\n", static_cast<int32_t>(value));       
    
    param[0] = Any(1);
    param[1] = Any(0.0);
    param[2] = Any(2);
    param[3] = Any(3);
    param[4] = Any(4);
    param[5] = Any(5);
    param[6] = Any(6);
    param[7] = Any(7);
    param[8] = Any(8);
    param[9] = Any(9);
    param[10] = Any(1.0);
    param[11] = Any(2.0);
    param[12] = Any(3.0);
    param[13] = Any(4.0);
    param[14] = Any(5.0);
    param[15] = Any(6.0);
    param[16] = Any(7.0);
    param[17] = Any(8.0);
    param[18] = Any(10);
    value = apply(19, param, reinterpret_cast<int64_t (*)()>(returnLongLong));
    assert(value.getType() == Any::TypeLongLong);
    printf("int64 value: %lld\n", static_cast<int64_t>(value));

    param[0] = Any(12);
    param[1] = Any(2);
    value = apply(2, param, reinterpret_cast<Any (*)()>(returnVariant));
    assert(value.getType() == Any::TypeLong);
    assert(static_cast<int32_t>(value) == 24);
    printf("int32 value: %d\n", static_cast<int32_t>(value));

    param[0] = false;
    value = apply(1, param, reinterpret_cast<float (*)()>(returnFloat));
    assert(value.getType() == Any::TypeFloat);
    printf("float value: %g\n", static_cast<float>(value));

    param[0] = Any(1);
    param[1] = Any(2);
    param[2] = Any(3);
    param[3] = Any(4);
    param[4] = Any(5);
    param[5] = Any(37);
    param[5].makeVariant();
    value = apply(6, param, reinterpret_cast<uint8_t (*)()>(returnOctet));
    assert(value.getType() == Any::TypeOctet);
    printf("octet value: %c\n", static_cast<uint8_t>(value));

    vh.setVar(200LL);
    value = vh.getVar(buf, sizeof buf);
    assert(value.getType() == Any::TypeLongLong);
    printf("int64 value: %lld\n", static_cast<int64_t>(value));

    vh.setVar(100);
    value = vh.getVar(buf, sizeof buf);
    assert(value.getType() == Any::TypeLong);
    printf("int32 value: %d\n", static_cast<int32_t>(value));

    vh.setVar("rgb(255,255,255)");
    value = vh.getVar(buf, sizeof buf);
    assert(value.getType() == Any::TypeString);
    printf("string value: %s\n", static_cast<const char*>(value));

    bool caught = false;
    try
    {
        value = apply(0, 0, reinterpret_cast<int32_t (*)()>(raise));
    }
    catch (...)
    {
        caught = true;
        printf("caught\n");
    }
    assert(caught);

    param[0] = Any(static_cast<intptr_t>(0));
    param[1] = Any(10414200LL);
    param[2] = Any(3u);
    param[3] = Any(1u);
    param[4] = Any(0LL);
    value = apply(5, param, reinterpret_cast<intptr_t (*)()>(map));
    printf("map: %u\n", value.getType());

    printf("done.\n");
}
