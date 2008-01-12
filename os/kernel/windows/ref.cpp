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

#include <es/ref.h>
#ifdef WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400 // Windows NT 4.0 for fiber
#endif  // _WIN32_WINNT
#include <windows.h>
#endif  // WIN32

#ifdef WIN32

long Ref::addRef(void)
{
    return InterlockedIncrement(&count);
}

long Ref::release(void)
{
    return InterlockedDecrement(&count);
}

#endif  // WIN32
