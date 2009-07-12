/*
 * Copyright 2008 Google Inc.
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

#include "importedObject.h"

namespace
{

NPObject* allocate(NPP npp, NPClass* aClass)
{
    return ImportedObject::getLastAllocated();
}

void deallocate(NPObject* object)
{
    delete static_cast<ImportedObject*>(object);
}

void invalidate(NPObject* object)
{
    return static_cast<ImportedObject*>(object)->invalidate();
}

bool hasMethod(NPObject* object, NPIdentifier name)
{
    return static_cast<ImportedObject*>(object)->hasMethod(name);
}

bool invoke(NPObject* object, NPIdentifier name, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    return static_cast<ImportedObject*>(object)->invoke(name, args, argCount, result);
}

bool invokeDefault(NPObject* object, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    return static_cast<ImportedObject*>(object)->invokeDefault(args, argCount, result);
}

bool hasProperty(NPObject* object, NPIdentifier name)
{
    return static_cast<ImportedObject*>(object)->hasProperty(name);
}

bool getProperty(NPObject* object, NPIdentifier name, NPVariant* result)
{
    return static_cast<ImportedObject*>(object)->getProperty(name, result);
}

bool setProperty(NPObject* object, NPIdentifier name, const NPVariant* value)
{
    return static_cast<ImportedObject*>(object)->setProperty(name, value);
}

bool removeProperty(NPObject* object, NPIdentifier name)
{
    return static_cast<ImportedObject*>(object)->removeProperty(name);
}

bool enumeration(NPObject* object, NPIdentifier* *value, uint32_t* count)
{
    return static_cast<ImportedObject*>(object)->enumeration(value, count);
}

bool construct(NPObject* object, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    return static_cast<ImportedObject*>(object)->construct(args, argCount, result);
}

}   // namespace

NPClass ImportedObject::npClass =
{
    NP_CLASS_STRUCT_VERSION_CTOR,
    ::allocate,
    ::deallocate,
    ::invalidate,
    ::hasMethod,
    ::invoke,
    ::invokeDefault,
    ::hasProperty,
    ::getProperty,
    ::setProperty,
    ::removeProperty,
    ::enumeration,
    ::construct
};

NPObject* ImportedObject::lastAllocated;
