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

#ifndef NINTENDO_ES_UTF_H_INCLUDED
#define NINTENDO_ES_UTF_H_INCLUDED

#include <string.h>
#include <es/types.h>

#ifdef __cplusplus
extern "C" {
#endif

char* utf8to32(const char* utf8, u32* utf32);
char* utf32to8(u32 utf32, char* utf8);
size_t utf32to8len(u32 utf32);
u16* utf16to32(const u16* utf16, u32* utf32);
u16* utf32to16(u32 utf32, u16* utf16);
u32 utftolower(u32 utf32);
u32 utftoupper(u32 utf32);
int utf16cmp(const u16* a, const u16* b);
int utf16icmp(const u16* a, const u16* b);
int utf16ncmp(const u16* a, const u16* b, size_t len);
int utf16nicmp(const u16* a, const u16* b, size_t len);
u16* utf16cpy(u16* a, const u16* b);
u16* utf16ncpy(u16* a, const u16* b, size_t len);
size_t utf16len(const u16* s);

char* utf16cpy8(char* a, const u16* b);
char* utf16ncpy8(char* a, const u16* b, size_t len);

#ifdef __cplusplus
}
#endif

#endif  // #ifndef NINTENDO_ES_UTF_H_INCLUDED
