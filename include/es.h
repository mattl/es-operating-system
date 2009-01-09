/*
 * Copyright 2008, 2009 Google Inc.
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

#ifndef NINTENDO_ES_H_INCLUDED
#define NINTENDO_ES_H_INCLUDED

#include <stdarg.h>
#include <string.h>
#include <es/types.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void esPanic(const char* file, int line, const char* msg, ...);
int  esReport(const char* spec, ...) __attribute__ ((format (printf, 1, 2)));
int  esReportv(const char* spec, va_list list) __attribute__ ((format (printf, 1, 0)));
void esDump(const void* ptr, s32 len);
void esSleep(s64 timeout);
void esThrow(int result);

#ifdef __es__

#ifndef NDEBUG

#ifndef ASSERT
#define ASSERT(exp)                                             \
    (void) ((exp) ||                                            \
            (esPanic(__FILE__, __LINE__, "Failed assertion " #exp), 0))
#endif

#ifndef ASSERTMSG
#define ASSERTMSG(exp, ...)                                     \
    (void) ((exp) ||                                            \
            (esPanic(__FILE__, __LINE__, __VA_ARGS__), 0))
#endif

#else   // !NDEBUG

#ifndef ASSERT
#define ASSERT(exp)             ((void) 0)
#endif

#ifndef ASSERTMSG
#define ASSERTMSG(exp, ...)     ((void) 0)
#endif

#endif  // !NDEBUG

#else   // !__es__

#include <assert.h>
#define ASSERT  assert

#endif  // !__es__

// Misc.
long long rand48(void);

#ifdef __cplusplus
}
#endif

#endif  // #ifndef NINTENDO_ES_H_INCLUDED
