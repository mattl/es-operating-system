/*
 * Copyright (c) 2006, 2007
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

#ifndef NINTENDO_ES_LIBES_CORE_H_INCLUDED
#define NINTENDO_ES_LIBES_CORE_H_INCLUDED

#include <es.h>

#ifdef __cplusplus

#include <es/base/IThread.h>

int esInit(IInterface** nameSpace);
IThread* esCreateThread(void* (*start)(void* param), void* param);

extern "C" {
#endif

size_t strnlen(const char *s, const size_t n);
int stricmp(const char *s1, const char *s2);
int strnicmp(const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
}
#endif

#endif // NINTENDO_ES_LIBES_CORE_H_INCLUDED
