/*
 * Copyright 2008, 2009 Google Inc.
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

//
// IBinding
//

#include <errno.h>
#include <string.h>
#include <es/formatter.h>
#include <es/handle.h>
#include "iso9660Stream.h"

Object* Iso9660Stream::
getObject()
{
    addRef();
    return static_cast<es::Context*>(this);
}

void Iso9660Stream::
setObject(Object* object)
{
    esThrow(EACCES); // [check] appropriate?
}

// Note getName() is implemented in "iso9660Ucs2.cpp" and in
// "iso9660Ascii.cpp".
