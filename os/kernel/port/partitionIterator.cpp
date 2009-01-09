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

// Disk partition manager. Since this manager can be applied to both
// ATA hard disks and SCSI hard disks, this manager is implemented
// separately from the hard disk device drivers.

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <es.h>
#include <es/exception.h>
#include <es/handle.h>
#include "partition.h"

using namespace LittleEndian;
using namespace es;

//
// PartitionIterator
//

PartitionIterator::
PartitionIterator(PartitionContext* context, int ipos) :
    context(context), ipos(ipos)
{
    context->addRef();
}

PartitionIterator::
~PartitionIterator()
{
    context->release();
}

bool PartitionIterator::
hasNext()
{
    Monitor::Synchronized method(context->monitor);

    PartitionStreamList::Iterator iter = context->partitionList.begin();
    int i = 0;
    PartitionStream* stream;
    while (stream = iter.next())
    {
        if (++i == ipos + 1)
        {
            return true;
        }
    }
    return false;
}

IInterface* PartitionIterator::
next()
{
    Monitor::Synchronized method(context->monitor);

    PartitionStreamList::Iterator iter = context->partitionList.begin();
    int i = 0;
    PartitionStream* stream;
    while (stream = iter.next())
    {
        if (++i == ipos + 1)
        {
            ++ipos;
            addRef();
            return static_cast<IBinding*>(this);
        }
    }
    return 0;
}

int PartitionIterator::
remove()
{
    // remove() removes the previous item.
    PartitionStreamList::Iterator iter = context->partitionList.begin();
    char name[PartitionContext::MAX_PREFIX_LEN + 4];
    Handle<IBinding> binding;

    --ipos;           // ipos is decremented in order to get the previous item.
    binding = next(); // ipos is restored because next() increments ipos.

    if (binding->getName(name, sizeof(name)) < 0)
    {
        return -1;
    }

    long long ret;
    ret = context->unbind(name);
    if (ret == 0)
    {
        // If the previous item was successfully removed,
        // ipos must be decremented.
        --ipos;
    }
    // ipos indicates the same item as before, after all.
    return ret;
}

//
// PartitionIterator : IBinding
//

IInterface* PartitionIterator::
getObject()
{
    Monitor::Synchronized method(context->monitor);

    PartitionStreamList::Iterator iter = context->partitionList.begin();
    int i = 0;
    PartitionStream* stream;
    while (stream = iter.next())
    {
        if (++i == ipos)
        {
            stream->addRef();
            return static_cast<IStream*>(stream);
        }
    }
    return 0;
}

void PartitionIterator::
setObject(IInterface* object)
{
    esThrow(EACCES); // [check] appropriate?
}

int PartitionIterator::
getName(char* name, int len)
{
    Monitor::Synchronized method(context->monitor);

    PartitionStreamList::Iterator iter = context->partitionList.begin();
    int i = 0;
    PartitionStream* stream;
    while (stream = iter.next())
    {
        if (++i == ipos)
        {
            u8 id = stream->getId();
            const char* prefix;
            if (stream->isPrimaryPartition())
            {
                prefix = PartitionContext::PREFIX_PRIMARY;
            }
            else if (stream->isExtendedPartition())
            {
                prefix = PartitionContext::PREFIX_EXTENDED;
            }
            else if (stream->isLogicalPartition())
            {
                prefix = PartitionContext::PREFIX_LOGICAL;
            }
            else
            {
                return -1;
            }

            memset(name, len, 0);

            if (stream->isExtendedPartition())
            {
#ifdef WIN32
                return _snprintf(name, len, "%s", prefix);
#else
                return snprintf(name, len, "%s", prefix);
#endif
            }

#ifdef WIN32
            return _snprintf(name, len, "%s%u", prefix, id);
#else
            return snprintf(name, len, "%s%u", prefix, id);
#endif
            return 0;
        }
    }
    return -1;
}

//
// PartitionIterator : IInterface
//

void* PartitionIterator::
queryInterface(const char* riid)
{
    void* objectPtr;
    if (strcmp(riid, IIterator::iid()) == 0)
    {
        objectPtr = static_cast<IIterator*>(this);
    }
    else if (strcmp(riid, IBinding::iid()) == 0)
    {
        objectPtr = static_cast<IBinding*>(this);
    }
    else if (strcmp(riid, IInterface::iid()) == 0)
    {
        objectPtr = static_cast<IIterator*>(this);
    }
    else
    {
        return NULL;
    }
    static_cast<IInterface*>(objectPtr)->addRef();
    return objectPtr;
}

unsigned int PartitionIterator::
addRef()
{
    return ref.addRef();
}

unsigned int PartitionIterator::
release()
{
    unsigned int count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}
