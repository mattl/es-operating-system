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
// IBinding
//

#include <string.h>
#include <es/formatter.h>
#include <es/handle.h>
#include "iso9660Stream.h"

IInterface* Iso9660Stream::
getObject()
{
    addRef();
    return static_cast<IContext*>(this);
}

int Iso9660Stream::
setObject(IInterface* object)
{
    return -1;
}

// Note getName() is implemented in "iso9660Ucs2.cpp" and in
// "iso9660Ascii.cpp".
