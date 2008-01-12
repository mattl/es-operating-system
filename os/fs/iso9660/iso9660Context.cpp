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
IBinding* Iso9660Stream::
bind(const char* name, IInterface* object)
{
    return 0;
}

IContext* Iso9660Stream::
createSubcontext(const char* name)
{
    return 0;
}

int Iso9660Stream::
destroySubcontext(const char* name)
{
    return -1;
}

IInterface* Iso9660Stream::
lookup(const char* name)
{
    Iso9660Stream* stream(lookupPathName(name));
    if (!name || *name != 0)
    {
        return 0;
    }
    return static_cast<IContext*>(stream);
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

IIterator* Iso9660Stream::
list(const char* name)
{
    Handle<Iso9660Stream> stream(lookupPathName(name));
    if (!stream || !name || *name != 0)
    {
        return 0;
    }
    return new Iso9660Iterator(stream);
}
