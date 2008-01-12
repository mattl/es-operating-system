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

#ifndef EVENTMANAGER_H_INCLUDED
#define EVENTMANAGER_H_INCLUDED

#include "IEventQueue.h"
#include <es/base/IClassStore.h>

#ifdef __cplusplus
extern "C" {
#endif

/** <code>0d1d9843-fd06-4bd0-b6fc-34bb08b5af09</code>
 */
const Guid CLSID_EventManager =
{
    0x0d1d9843, 0xfd06, 0x4bd0, { 0xb6, 0xfc, 0x34, 0xbb, 0x08, 0xb5, 0xaf, 0x09 }
};

extern unsigned char IEventQueueInfo[];
extern unsigned IEventQueueInfoSize;

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // EVENTMANAGER_H_INCLUDED
