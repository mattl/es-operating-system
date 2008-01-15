/*
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
