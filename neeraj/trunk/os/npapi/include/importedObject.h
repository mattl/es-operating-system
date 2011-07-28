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

#ifndef GOOGLE_ES_NPAPI_IMPORTED_OBJECTS_H_INCLUDED
#define GOOGLE_ES_NPAPI_IMPORTED_OBJECTS_H_INCLUDED

#include "npapi.h"
#include "npruntime.h"
#include "npupp.h"

#include <es/capability.h>
#include <es/objectTable.h>
#include <es/ref.h>
#include <es/uuid.h>

struct ImportKey
{
    NPP npp;
    const es::Capability capability;
    const Guid& iid;

public:
    ImportKey(NPP npp, const es::Capability capability, const Guid& iid) :
        npp(npp),
        capability(capability),
        iid(iid)
    {
    }

    size_t hash() const
    {
        return capability.hash();
    }
};

class ImportedObject : public NPObject
{
    static NPClass npClass;
    static NPObject* lastAllocated;

    Ref ref;
    es::Capability capability;
    const Guid iid;
    bool doRelease;

public:
    ImportedObject(const ImportKey& key) :
        ref(0),
        iid(iid),
        doRelease(false)
    {
        capability.copy(key.capability);

        lastAllocated = this;
        NPN_CreateObject(key.npp, &npClass);
    }

    ~ImportedObject()
    {
    }

    bool isMatch(const ImportKey& key) const
    {
        return (capability == key.capability) ? true : false;
    }

    size_t hash() const
    {
        return capability.hash();
    }

    unsigned int addRef()
    {
        doRelease = true;
        return ref.addRef();
    }

    unsigned int release()
    {
        return ref.release();
    }

    void invalidate();
    bool hasMethod(NPIdentifier name);
    // Determine the type of interface and which method is being invoked.
    // Reflect::Interface interface = getInterface(iid);
    bool invoke(NPIdentifier name, const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool invokeDefault(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool hasProperty(NPIdentifier name);
    bool getProperty(NPIdentifier name, NPVariant* result);
    bool setProperty(NPIdentifier name, const NPVariant* value);
    bool removeProperty(NPIdentifier name);
    bool enumeration(NPIdentifier* *value, uint32_t* count);
    bool construct(const NPVariant* args, uint32_t argCount, NPVariant* result);

    static NPObject* getLastAllocated()
    {
        return lastAllocated;
    }
};

#endif  // GOOGLE_ES_NPAPI_IMPORTED_OBJECTS_H_INCLUDED
