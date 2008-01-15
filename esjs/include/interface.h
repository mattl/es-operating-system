/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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
