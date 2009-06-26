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

#ifndef GOOGLE_ES_CAPABILITY_H_INCLUDED
#define GOOGLE_ES_CAPABILITY_H_INCLUDED

#include <stdio.h>
#include <es/types.h>

namespace es
{

struct Capability
{
    int     pid;
    int     object;
    u64     check;

    size_t hash() const
    {
        return hash(pid, object, check);
    }

    Capability& copy(const Capability& other)
    {
        pid = other.pid;
        object = other.object;
        check = other.check;
        return *this;
    }

    static size_t hash(pid_t pid, int object, u64 check)
    {
        return static_cast<size_t>(pid ^ object ^ check);
    }

    void report()
    {
        printf("Capability: <%d, %d, %llx>\n", pid, object, check);
    }
};

inline int operator==(const Capability& c1, const Capability& c2)
{
   return (c1.pid == c2.pid &&
           c1.object == c2.object &&
           c1.check == c2.check) ? true : false;
}

}   // namespace es

#endif  // GOOGLE_ES_CAPABILITY_H_INCLUDED
