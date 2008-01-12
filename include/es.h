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

#ifndef NINTENDO_ES_H_INCLUDED
#define NINTENDO_ES_H_INCLUDED

#include <stdarg.h>
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

typedef struct esOnceControl
{
    int  done;
    long started;
} esOnceControl;

#define ES_ONCE_INIT {0, -1}

int   esOnce(esOnceControl* control, void (*func)(void));

int   esCreateThreadKey(unsigned int* key, void (*dtor)(void*));
int   esDeleteThreadKey(unsigned int key);
void* esGetThreadSpecific(unsigned int key);
int   esSetThreadSpecific(unsigned int key, const void* ptr);
void  esDeallocateSpecific(void);

typedef struct esMonitor
{
    void* monitor;
} esMonitor;

void esCreateMonitor(esMonitor* monitor);
void esLockMonitor(esMonitor* monitor);
int  esTryLockMonitor(esMonitor* monitor);
void esUnlockMonitor(esMonitor* monitor);

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
