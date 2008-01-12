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

#include <time.h>
#include <es/dateTime.h>
#include <es/timeSpan.h>

DateTime DateTime::getNow()
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    return DateTime(1970, 1, 1) + TimeSpan(ts.tv_sec * 10000000LL + ts.tv_nsec / 100);
}
