/*
 * Copyright 2008, 2009 Google Inc.
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

#ifndef GOOGLE_ES_NPAPI_EXPORTED_OBJECT_H_INCLUDED
#define GOOGLE_ES_NPAPI_EXPORTED_OBJECT_H_INCLUDED

#include "npapi.h"
#include "npruntime.h"
#include "npupp.h"

#include <es/objectTable.h>
#include <es/ref.h>
#include <es/rpc.h>
#include <es/uuid.h>

struct ExportKey
{
    NPObject* object;
    const char* iid;

public:
    ExportKey(NPObject* object, const char* iid) :
        object(object),
        iid(iid)
    {
    }

    size_t hash() const
    {
        return hash(object, iid);
    }

    static size_t hash(NPObject* object, const char* iid)
    {
        size_t hash = 0;
        while (*iid)
        {
            hash = (hash << 1) ^ *iid++;
        }
        hash ^= reinterpret_cast<size_t>(object);
        return hash;
    }
};

class ExportedObject
{
    Ref ref;
    NPObject* object;
    const char* iid;
    u64 check;
    bool doRelease;

public:
    ExportedObject(const ExportKey& key) :
        object(key.object),
        iid(key.iid),
        check(1),    // TODO: generate non-zero random number here
        doRelease(false)
    {
        NPN_RetainObject(object);
    }

    ~ExportedObject()
    {
        // Do release if necessary on objectTable
        if (doRelease)
        {
            NPN_ReleaseObject(object);
        }
    }

    bool isMatch(const ExportKey& key) const
    {
        return (object == key.object && strcmp(iid, key.iid) == 0) ? true : false;
    }

    u64 getCheck() const
    {
        return check;
    }

    size_t hash() const
    {
        return ExportKey::hash(object, iid);
    }

    unsigned int addRef()
    {
        doRelease = true;   // TODO: How NPN_RetainObject is called??
        return ref.addRef();
    }

    unsigned int release()
    {
        return ref.release();
    }

    // called by the focus thread
    int invoke(es::RpcReq* req, int* fdv, int* fdmax, int s);

    // called by the plugin thread
    int asyncInvoke(void* param);
};

#endif  // GOOGLE_ES_NPAPI_EXPORTED_OBJECT_H_INCLUDED
