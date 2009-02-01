/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

/*
 * These coded instructions, statements, and computer programs contain
 * software derived from Squeak.
 *
 *   Squeak is distributed for use and modification subject to a liberal
 *   open source license.
 *
 *   http://www.squeak.org/SqueakLicense/
 *
 *   Unless stated to the contrary, works submitted for incorporation
 *   into or for distribution with Squeak shall be presumed subject to
 *   the same license.
 *
 *   Portions of Squeak are:
 *
 *   Copyright (c) 1996 Apple Computer, Inc.
 *   Copyright (c) 1997-2001 Walt Disney Company, and/or
 *   Copyrighted works of other contributors.
 *   All rights reserved.
 */

#include <es.h>
#include <es/dateTime.h>
#include <es/base/IFile.h>
#include <es/base/IStream.h>
#include <es/util/IIterator.h>
#include <es/naming/IBinding.h>
#include <es/naming/IContext.h>
#include <es/handle.h>



extern Handle<es::Context> gRoot;

extern "C"
{
    #include "sq.h"
    #include "FilePlugin.h"
}

/* End of adjustments for pluginized VM */

/***
    The interface to the directory primitive is path based.
    That is, the client supplies a Squeak string describing
    the path to the directory on every call. To avoid traversing
    this path on every call, a cache is maintained of the last
    path seen, along with the Mac volume and folder reference
    numbers corresponding to that path.
***/

/*** Constants ***/
#define ENTRY_FOUND     0
#define NO_MORE_ENTRIES 1
#define BAD_PATH        2

#define DELIMITOR '/'
#define MAX_PATH 2000

static const DateTime Epoch(1901, 1, 1);

/*** Variables ***/
extern struct VirtualMachine * interpreterProxy;

/*** Functions ***/
static int convertToSqueakTime(long long tick)
{
    return (int) ((tick - Epoch.getTicks()) / 10000000);
}

/* Create a new directory with the given path. By default, this
   directory is created in the current directory. Use
   a full path name such as "MyDisk:Working:New Folder" to
   create folders elsewhere. */
int dir_Create(char *pathString, int pathStringLength)
{
    char cPathName[1001];

    // copy the file name into a null-terminated C string
    if (sizeof cPathName <= pathStringLength)
    {
        return false;
    }
    sqFilenameFromString(cPathName, pathString, pathStringLength);

    es::Context* subcontext;
    subcontext = gRoot->createSubcontext(cPathName);
    if (!subcontext)
    {
        return false;
    }
    subcontext->release();
    return true;
}

/* Delete the existing directory with the given path. */
int dir_Delete(char *pathString, int pathStringLength)
{
    char cPathName[1001];

    // copy the file name into a null-terminated C string
    if (sizeof cPathName <= pathStringLength)
    {
        return false;
    }
    sqFilenameFromString(cPathName, pathString, pathStringLength);

    if (gRoot->destroySubcontext(cPathName) < 0)
    {
        return false;
    }
    return true;
}

int dir_Delimitor(void)
{
    return DELIMITOR;
}

/* Lookup the index-th entry of the directory with the given path, starting
   at the root of the file system. Set the name, name length, creation date,
   creation time, directory flag, and file size (if the entry is a file).
   Return:  0   if a entry is found at the given index
            1   if the directory has fewer than index entries
            2   if the given path has bad syntax or does not reach a directory
*/
int dir_Lookup(char *pathString, int pathStringLength, int index /* starting at 1 */,
               // outputs:
               char *name, int *nameLength, int *creationDate, int *modificationDate,
               int *isDirectory, squeakFileOffsetType *sizeIfFile)
{
    /* default return values */
    *name             = 0;
    *nameLength       = 0;
    *creationDate     = 0;
    *modificationDate = 0;
    *isDirectory      = false;
    *sizeIfFile       = 0;

    // copy the file name into a null-terminated C string
    char cPathName[1001];
    if (sizeof cPathName <= pathStringLength)
    {
        return BAD_PATH;
    }
    sqFilenameFromString(cPathName, pathString, pathStringLength);

    /* get file or directory info */
    int i;
    Handle<es::Iterator> iter(gRoot->list(cPathName));
    if (!iter)
    {
        return BAD_PATH;
    }
    for (i = 1; iter->hasNext(); ++i)
    {
        Handle<es::Binding> binding(iter->next());
        if (i == index)
        {
            binding->getName(name, MAX_PATH);
            *nameLength = strlen(name);

            Handle<es::File> file(binding);
            if (file)
            {
                long long tick;
                tick = file->getCreationTime();
                *creationDate = convertToSqueakTime(tick);
                tick = file->getLastWriteTime();
                *modificationDate = convertToSqueakTime(tick);
            }

            Handle<es::Context> context(binding);
            if (context)
            {
                *isDirectory = true;
                *sizeIfFile = 0;
            }
            else
            {
                Handle<es::Stream> stream(file->getStream());  // XXX Check exception
                if (stream)
                {
                    *isDirectory = false;
                    long long size;
                    size = stream->getSize();
                    *sizeIfFile = size;
                }
                else
                {
                    ++index;
                    continue;
                }
            }
            return ENTRY_FOUND;
        }
    }
    return NO_MORE_ENTRIES;
}

int dir_SetMacFileTypeAndCreator(char *filename, int filenameSize, char *fType, char *fCreator)
{
    // ES files are untyped, and the creator is correct by default
    return true;
}

int dir_GetMacFileTypeAndCreator(char *filename, int filenameSize, char *fType, char *fCreator)
{
    // Win32 files are untyped, and the creator is correct by default
    return interpreterProxy->primitiveFail();
}
