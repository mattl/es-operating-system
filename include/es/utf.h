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
