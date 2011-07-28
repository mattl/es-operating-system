/*
 * Copyright 2010 Esrille Inc.
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

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

char* utf8to32(const char* utf8, uint32_t* utf32);
char* utf32to8(uint32_t utf32, char* utf8);
size_t utf32to8len(uint32_t utf32);
uint16_t* utf16to32(const uint16_t* utf16, uint32_t* utf32);
uint16_t* utf32to16(uint32_t utf32, uint16_t* utf16);
uint32_t utftolower(uint32_t utf32);
uint32_t utftoupper(uint32_t utf32);
int utf16cmp(const uint16_t* a, const uint16_t* b);
int utf16icmp(const uint16_t* a, const uint16_t* b);
int utf16ncmp(const uint16_t* a, const uint16_t* b, size_t len);
int utf16nicmp(const uint16_t* a, const uint16_t* b, size_t len);
uint16_t* utf16cpy(uint16_t* a, const uint16_t* b);
uint16_t* utf16ncpy(uint16_t* a, const uint16_t* b, size_t len);
size_t utf16len(const uint16_t* s);

char* utf16cpy8(char* a, const uint16_t* b);
char* utf16ncpy8(char* a, const uint16_t* b, size_t len);

#ifdef __cplusplus
}
#endif

#endif  // #ifndef NINTENDO_ES_UTF_H_INCLUDED
