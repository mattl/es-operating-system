/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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
