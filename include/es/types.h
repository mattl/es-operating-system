/*
 * Copyright (c) 2006
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

#ifndef NINTENDO_ES_TYPES_H_INCLUDED
#define NINTENDO_ES_TYPES_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long long  u64;
typedef   signed long long  s64;
typedef unsigned int        u32;
typedef   signed int        s32;
typedef unsigned short      u16;
typedef   signed short      s16;
typedef unsigned char       u8;
typedef   signed char       s8;
typedef float               f32;
typedef double              f64;
typedef long double         f96;

#ifdef __cplusplus
}
#endif

#ifndef NULL
#ifdef  __cplusplus
#define NULL                0
#else   // __cplusplus
#define NULL                ((void*) 0)
#endif  // __cplusplus
#endif  // NULL

#endif  // #ifndef NINTENDO_ES_TYPES_H_INCLUDED
