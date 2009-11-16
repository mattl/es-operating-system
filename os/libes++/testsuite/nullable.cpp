/*
 * Copyright 2009 Google Inc.
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
#include <string>
#include <es/nullable.h>

int main()
{
    Nullable<int> a;
    Nullable<int> b(5);
    Nullable<std::string> c;
    Nullable<std::string> d("hello");
    printf("%d %d\n", a.hasValue(), a.hasValue() ? a.value() : 0);
    printf("%d %d\n", b.hasValue(), b.hasValue() ? b.value() : 0);
    printf("%d %s\n", c.hasValue(), c.hasValue() ? c.value().c_str() : "");
    printf("%d %s\n", d.hasValue(), d.hasValue() ? d.value().c_str() : "");

    try
    {
        int x = a.value();
    }
    catch (SystemException<ENODATA> e)
    {
        printf("%d\n", e.getResult());
    }
}
