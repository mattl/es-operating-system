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

Variant returnVariant()
{
    return Variant(10);
}

uint8_t returnOctet()
{
    return 'c';
}

float returnFloat()
{
    return 10.0f;
}

long long returnLongLong(int a, double b, const char* c, unsigned long long d)
{
    printf("returnLongLong(%d, %g, %s, %llu)\n", a, b, c, d);
    return 200LL;
}

int main()
{
    VariantHolder vh;
    char buf[128];
    Variant value;

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

    value = apply(0, NULL, returnVariant);
    assert(value.getType() == Variant::TypeLong);
    printf("int32 value: %d\n", static_cast<int32_t>(value));

    value = apply(0, NULL, returnFloat);
    assert(value.getType() == Variant::TypeFloat);
    printf("float value: %g\n", static_cast<float>(value));

    value = apply(0, NULL, returnOctet);
    assert(value.getType() == Variant::TypeOctet);
    printf("octet value: %c\n", static_cast<uint8_t>(value));

    Variant param[4];
    param[0] = Variant(10);
    param[1] = Variant(20.0);
    param[2] = Variant("hello");
    param[3] = Variant(500LLu);
    value = apply(sizeof param / sizeof param[0], param, reinterpret_cast<int64_t (*)()>(returnLongLong));
    assert(value.getType() == Variant::TypeLongLong);
    printf("int64 value: %lld\n", static_cast<int64_t>(value));

    printf("done.\n");
}
