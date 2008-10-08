/*
 * Copyright 2008 Google Inc.
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
#include <es/variant.h>

class VariantHolder
{
    static const int STRING_CHAR_MAX = 128;

    Variant var;
    char* string;
    int length;

public:
    VariantHolder()
    {
    }

    ~VariantHolder()
    {
        if (var.getType() == Variant::TypeString && string)
        {
            delete string;
        }
    }

    Variant getVar(void* value, int valueLength)
    {
        int type = var.getType();
        if (type == Variant::TypeString)
        {
            strncpy(static_cast<char*>(value), string, valueLength);
            return static_cast<const char*>(value);
        }
        return var;
    }

    int setVar(Variant value)
    {
        int type = value.getType();
        if (type == Variant::TypeString)
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

Variant returnVariant(int x, int y)
{
    return Variant(x * y);
}

uint8_t returnOctet()
{
    return 'c';
}

float returnFloat()
{
    return 10.0f;
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

int main()
{
    VariantHolder vh;
    char buf[128];
    Variant param[19];
    Variant value;

    printf("sizeof(Variant) = %u, offsetof(VariantBase, type) = %u\n", sizeof(Variant), offsetof(VariantBase, type));

    value = returnVariant(2, 3);
    assert(value.getType() == Variant::TypeLong);
    assert(static_cast<int32_t>(value) == 6);
    printf("%d\n", evaluate(value));

    returnLongLong(1, 0.0, 2, 3, 4, 5, 6, 7, 8, 9,
                   1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 10);

    param[0] = Variant(12);
    param[1] = Variant(2);
    value = apply(2, param, reinterpret_cast<Variant (*)()>(returnVariant));
    assert(value.getType() == Variant::TypeLong);
    assert(static_cast<int32_t>(value) == 24);
    printf("int32 value: %d\n", static_cast<int32_t>(value));

    param[0] = Variant(1);
    param[1] = Variant(0.0);
    param[2] = Variant(2);
    param[3] = Variant(3);
    param[4] = Variant(4);
    param[5] = Variant(5);
    param[6] = Variant(6);
    param[7] = Variant(7);
    param[8] = Variant(8);
    param[9] = Variant(9);
    param[10] = Variant(1.0);
    param[11] = Variant(2.0);
    param[12] = Variant(3.0);
    param[13] = Variant(4.0);
    param[14] = Variant(5.0);
    param[15] = Variant(6.0);
    param[16] = Variant(7.0);
    param[17] = Variant(8.0);
    param[18] = Variant(10);
    value = apply(19, param, reinterpret_cast<int64_t (*)()>(returnLongLong));
    assert(value.getType() == Variant::TypeLongLong);
    printf("int64 value: %lld\n", static_cast<int64_t>(value));

    value = apply(0, NULL, returnFloat);
    assert(value.getType() == Variant::TypeFloat);
    printf("float value: %g\n", static_cast<float>(value));

    value = apply(0, NULL, returnOctet);
    assert(value.getType() == Variant::TypeOctet);
    printf("octet value: %c\n", static_cast<uint8_t>(value));

    vh.setVar(200LL);
    value = vh.getVar(buf, sizeof buf);
    assert(value.getType() == Variant::TypeLongLong);
    printf("int64 value: %lld\n", static_cast<int64_t>(value));

    vh.setVar(100);
    value = vh.getVar(buf, sizeof buf);
    assert(value.getType() == Variant::TypeLong);
    printf("int32 value: %d\n", static_cast<int32_t>(value));

    vh.setVar("rgb(255,255,255)");
    value = vh.getVar(buf, sizeof buf);
    assert(value.getType() == Variant::TypeString);
    printf("string value: %s\n", static_cast<const char*>(value));

    printf("done.\n");
}
