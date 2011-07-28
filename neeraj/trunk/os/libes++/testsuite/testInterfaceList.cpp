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

#include <es/interfaceData.h>
#include <es/reflect.h>

#include <stdio.h>

int main()
{
    int n = 0;
    for (es::InterfaceData* data = es::interfaceData; data->iid; ++data, ++n)
    {
        printf("%d: %s %s\n", n, data->iid(), data->info());
        Reflect::Interface meta(data->info(), data->iid());
    }
}
