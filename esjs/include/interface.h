/*
 * Copyright (c) 2007
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

#ifndef NINTENDO_ESJS_INTERFACE_H_INCLUDED
#define NINTENDO_ESJS_INTERFACE_H_INCLUDED

#include <es.h>
#include <es/reflect.h>
#include <es/apply.h>

class ObjectValue;

Reflect::Interface& getInterface(const Guid& iid);

ObjectValue* constructInterfaceObject();
ObjectValue* constructSystemObject(void* system);

#endif // NINTENDO_ESJS_INTERFACE_H_INCLUDED
