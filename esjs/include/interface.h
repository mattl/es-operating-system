/*
 * Copyright 2008, 2009 Google Inc.
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
#include <es/any.h>
#include <es/reflect.h>
#include <es/object.h>

#include "esjs.h"

namespace es
{
    Reflect::Interface& getInterface(const char* iid);
    Object* getConstructor(const char* iid);
}

ObjectValue* constructInterfaceObject();
ObjectValue* constructSystemObject(void* system);

//
// InterfacePointerValue
//

class InterfacePointerValue : public ObjectValue
{
    Object* object;

public:
    InterfacePointerValue(Object* object) :
        object(object)
    {
    }

    ~InterfacePointerValue()
    {
        if (object)
        {
            object->release();
        }
    }

    Object*& getObject()
    {
        return object;
    }

    void clearObject()
    {
        object = 0;
    }

    Value* get(const std::string& name);
    void put(const std::string& name, Value* value, int attributes = 0);
};

#endif // NINTENDO_ESJS_INTERFACE_H_INCLUDED
