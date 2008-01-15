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

#ifndef BINDER_H_INCLUDED
#define BINDER_H_INCLUDED

#include <es/base/IClassStore.h>

#ifdef __cplusplus
extern "C" {
#endif

/** <code>b0f595e9-bd50-4146-8df2-ff61024f5e1b</code>
 */
const Guid CLSID_Binder =
{
    0xb0f595e9, 0xbd50, 0x4146, { 0x8d, 0xf2, 0xff, 0x61, 0x02, 0x4f, 0x5e, 0x1b }
};

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // #ifndef BINDER_H_INCLUDED
