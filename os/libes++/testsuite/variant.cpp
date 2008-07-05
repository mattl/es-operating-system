/*
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

#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <assert.h>
#include <es/variant.h>

#define BUFLENMAX 128

class VariantHolder
{
    Variant var;
public:
    VariantHolder()
    {
    }

    ~VariantHolder()
    {
        if (var.getType() == Variant::VTypeString && stringBuf)
        {
            delete stringBuf;
        }
        else if (var.getType() == Variant::VTypeWString && wstringBuf)
        {
            delete wstringBuf;
        }
    }

    Variant getVar(void* value, int valueLength)
    {
        Variant::VariantType vType = var.getType();
        if (vType == Variant::VTypeString)
        {
            int length = std::min(buflen, valueLength);
            strncpy(reinterpret_cast<char*>(value), stringBuf, length);
            return stringBuf;
        }
        else if (vType == Variant::VTypeWString)
        {
            int length = std::min(buflen, valueLength);
            wcsncpy(reinterpret_cast<wchar_t*>(value), wstringBuf, length);
            return wstringBuf;
        }
        return var;
    }

    int setVar(Variant value)
    {
        Variant::VariantType vType = value.getType();
        if (vType == Variant::VTypeString)
        {
            char* ptr = static_cast<char*>(value);
            buflen = std::min((int)strlen(ptr)+1, BUFLENMAX);
            stringBuf = new char[buflen];
            strncpy(stringBuf, ptr, buflen);
            var = stringBuf;
        }
        else if (vType == Variant::VTypeWString)
        {
            wchar_t* ptr = static_cast<wchar_t*>(value);
            for (buflen = 0; buflen < BUFLENMAX; ++buflen)
            {
                if (ptr[buflen] == L'\0')
                {
                    break;
                }
            }
            buflen++;
            wstringBuf = new wchar_t[buflen];
            wcsncpy(wstringBuf, ptr, buflen);
            var = wstringBuf;
        }
        else
        {
            var = value;
            buflen = 0;
        }
        return buflen;
    }

private:
    union {
        char* stringBuf;
        wchar_t* wstringBuf;
    };
    int buflen;
};

int main()
{
    VariantHolder vh;
    char buf[128];
    Variant value;

    vh.setVar(100);
    value = vh.getVar(buf, sizeof(buf));
    printf("int value: %d\n", static_cast<int>(value));
    vh.setVar("rgb(255,255,255)");
    value = vh.getVar(buf, sizeof(buf));
    printf("string value: %s\n", buf);
    vh.setVar(L"abc");
    value = vh.getVar(buf, sizeof(buf));
    printf("wstring value: %ls\n", reinterpret_cast<wchar_t*>(buf));
    printf("done.\n");
}
