/*
 * Copyright (c) 2007
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
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
