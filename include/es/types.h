/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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
typedef long double         f128;

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
