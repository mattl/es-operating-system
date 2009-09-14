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
// IContext
//

#include <string.h>
#include <es.h>
#include <es/formatter.h>
#include <es/handle.h>
#include "iso9660Stream.h"

// object - In general, object is possibly null. For Iso9660Stream, however,
// object must be NULL.
es::Binding* Iso9660Stream::
bind(const char* name, Object* object)
{
    return 0;
}

es::Context* Iso9660Stream::
createSubcontext(const char* name)
{
    return 0;
}

int Iso9660Stream::
destroySubcontext(const char* name)
{
    return -1;
}

Object* Iso9660Stream::
lookup(const char* name)
{
    Iso9660Stream* stream(lookupPathName(name));
    if (!name || *name != 0)
    {
        return 0;
    }
    return static_cast<es::Context*>(stream);
}

int Iso9660Stream::
rename(const char* oldName, const char* newName)
{
    return -1;
}

int Iso9660Stream::
unbind(const char* name)
{
    return -1;
}

es::Iterator* Iso9660Stream::
list(const char* name)
{
    Handle<Iso9660Stream> stream(lookupPathName(name));
    if (!stream || !name || *name != 0)
    {
        return 0;
    }
    return new Iso9660Iterator(stream);
}
