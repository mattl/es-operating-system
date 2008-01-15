/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_APPLY_H_INCLUDED
#define NINTENDO_ES_APPLY_H_INCLUDED

#include <es.h>
#include <es/uuid.h>

// Function apply system

struct Param
{
    enum
    {
        S32,
        S64,
        F32,
        F64,
        PTR,
        REF
    };
    union
    {
        int         s32;
        long long   s64;
        float       f32;
        double      f64;
        const void* ptr;
        Guid        guid;
    };
    int             cls;    // S32, S64, F32, F64, PTR
};

extern "C" s32 applyS32(int argc, Param* argv, s32 (*function)());
extern "C" s64 applyS64(int argc, Param* argv, s64 (*function)());
extern "C" f32 applyF32(int argc, Param* argv, f32 (*function)());
extern "C" f64 applyF64(int argc, Param* argv, f64 (*function)());
extern "C" void* applyPTR(int argc, Param* argv, const void* (*function)());

#endif // NINTENDO_ES_APPLY_H_INCLUDED
