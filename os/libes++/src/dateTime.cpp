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

#include <es/dateTime.h>

const int DateTime::yearDays[13] =
{
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

const int DateTime::leapYearDays[13] =
{
    0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366
};
