/*
 * Copyright 2008 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

/*---------------------------------------------------------------------------*

  These coded instructions, statements, and computer programs contain
  security software derived from the RSA Data Security, Inc. MD5 Message-
  Digest Algorithm.

 *---------------------------------------------------------------------------*/

#ifndef NINTENDO_ES_MD5_H_INCLUDED
#define NINTENDO_ES_MD5_H_INCLUDED

#include <es/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MD5.H - header file for MD5C.C */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
 */

/* MD5 context. */
typedef struct MD5Context MD5_CTX;
typedef struct MD5Context MD5Context;
struct MD5Context
{
    u32 state[4];       // state (ABCD)
    u32 count[2];       // number of bits, modulo 2^64 (lsb first)
    u8  buffer[64];     // input buffer
};

void MD5Init  (MD5Context* context);
void MD5Update(MD5Context* context, const void* input, size_t inputLen);
void MD5Final (u8 digest[16], MD5Context* context);

#ifdef __cplusplus
}
#endif

#endif  // #ifndef NINTENDO_ES_MD5_H_INCLUDED
