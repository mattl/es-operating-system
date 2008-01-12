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

#ifndef LOCATION_H_INCLUDED
#define LOCATION_H_INCLUDED

#include <es/base/IClassStore.h>
#include "ILocation.h"

#ifdef __cplusplus
extern "C" {
#endif

/** <code>7bc74f7b-a42b-4df6-9be9-24daee701fcd</code>
 */
const Guid CLSID_Location =
{
    0x21451205, 0xafbe, 0x4d5b, { 0x91, 0xff, 0x27, 0xdc, 0x23, 0x50, 0x9f, 0x08 }
};

#ifdef __cplusplus
}
#endif  // __cplusplus

extern unsigned char ILocationInfo[];
extern unsigned ILocationInfoSize;

#endif  // #ifndef LOCATION_H_INCLUDED
