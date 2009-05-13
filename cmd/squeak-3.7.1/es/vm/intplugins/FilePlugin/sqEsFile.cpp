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
#include <es/handle.h>
#include <es/base/IFile.h>
#include <es/base/IStream.h>
#include <es/naming/IContext.h>



#ifndef _DEBUG
#define FPRINTF(...)    (__VA_ARGS__)
#else
#define FPRINTF(...)    esReport(__VA_ARGS__)
#endif

extern "C"
{
    #include "sq.h"
    #include "FilePlugin.h"
}

extern Handle<es::Context> gRoot;

static es::Stream* getStream(SQFile* f)
{
    return reinterpret_cast<es::Stream*>(f->file);
}

/***
    The state of a file is kept in the following structure,
    which is stored directly in a Squeak bytes object.
    NOTE: The Squeak side is responsible for creating an
    object with enough room to store sizeof(SQFile) bytes.

    The session ID is used to detect stale file objects--
    files that were still open when an image was written.
    The file pointer of such files is meaningless.

    Files are always opened in binary mode; Smalltalk code
    does (or someday will do) line-end conversion if needed.

    Writeable files are opened read/write. The stdio spec
    requires that a positioning operation be done when
    switching between reading and writing of a read/write
    filestream. The lastOp field records whether the last
    operation was a read or write operation, allowing this
    positioning operation to be done automatically if needed.

    typedef struct {
        File    *file;
        int     sessionID;
        int     writable;
        int     fileSize;
        int     lastOp;  // 0 = uncommitted, 1 = read, 2 = write //
    } SQFile;

***/

/*** Constants ***/
#define UNCOMMITTED 0
#define READ_OP     1
#define WRITE_OP    2

#ifndef SEEK_SET
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2
#endif

/*** Variables ***/
int thisSession = 0;
extern struct VirtualMachine * interpreterProxy;

// Return true if the file's read/write head is at the end of the file.
int sqFileAtEnd(SQFile* f)
{
    FPRINTF("%s(%p)\n", __func__, f);
    if (!sqFileValid(f))
    {
        FPRINTF("%s(%p) : failed.\n", __func__, f);
        return interpreterProxy->success(false);
    }

    es::Stream* stream = getStream(f);
    long long position;
    position = stream->getPosition();
    return position == f->fileSize;
}

// Close the given file.
int sqFileClose(SQFile* f)
{
    FPRINTF("%s(%p)\n", __func__, f);
    if (!sqFileValid(f))
    {
        FPRINTF("%s(%p) : failed.\n", __func__, f);
        return interpreterProxy->success(false);
    }

    es::Stream* stream = getStream(f);
    stream->release();

    f->file = NULL;
    f->sessionID = 0;
    f->writable = false;
    f->fileSize = 0;
    f->lastOp = UNCOMMITTED;

    return 1;
}

int sqFileDeleteNameSize(int sqFileNameIndex, int sqFileNameSize)
{
    FPRINTF("%s(%*.*s)\n", __func__, sqFileNameSize, sqFileNameSize, (char*) sqFileNameIndex);

    char cFileName[1000];
    int i;

    // copy the file name into a null-terminated C string
    if (sizeof cFileName <= sqFileNameSize)
    {
        FPRINTF("%s : failed.\n", __func__);
        return interpreterProxy->success(false);
    }
    sqFilenameFromString(cFileName, sqFileNameIndex, sqFileNameSize);

    if (gRoot->unbind(cFileName) < 0)
    {
        FPRINTF("%s : failed.\n", __func__);
        return interpreterProxy->success(false);
    }

    return 1;
}

// Return the current position of the file's read/write head.
squeakFileOffsetType sqFileGetPosition(SQFile* f)
{
    FPRINTF("%s(%p) : ", __func__, f);

    if (!sqFileValid(f))
    {
        FPRINTF("failed.\n");
        return interpreterProxy->success(false);
    }

    es::Stream* stream = getStream(f);
    long long position;
    position = stream->getPosition();
    FPRINTF("%d\n", position);
    return (squeakFileOffsetType) position;
}

/* Create a session ID that is unlikely to be repeated.
   Zero is never used for a valid session number.
   Should be called once at startup time.
 */
int sqFileInit(void)
{
    thisSession = 0;    // XXX
    if (thisSession == 0)
    {
        thisSession = 1;  // don't use 0
    }
    return 1;
}

int sqFileShutdown(void)
{
    return 1;
}

/* Opens the given file using the supplied sqFile structure
   to record its state. Fails with no side effects if f is
   already open. Files are always opened in binary mode;
   Squeak must take care of any line-end character mapping.
 */
int sqFileOpen(SQFile* f, int sqFileNameIndex, int sqFileNameSize, int writeFlag)
{
    char cFileName[1001];

    FPRINTF("%s(%p, \"%*.*s\", %d) : %d\n", __func__, f,
            sqFileNameSize, sqFileNameSize, (char *) sqFileNameIndex, writeFlag,
            sqFileValid(f));

    // don't open an already open file
    if (sqFileValid(f))
    {
        FPRINTF("%s: failed.\n", __func__);
        return interpreterProxy->success(false);
    }

    // copy the file name into a null-terminated C string
    if (sizeof cFileName <= sqFileNameSize)
    {
        FPRINTF("%s: failed.\n", __func__);
        return interpreterProxy->success(false);
    }
    sqFilenameFromString(cFileName, sqFileNameIndex, sqFileNameSize);

    Object* node = gRoot->lookup(cFileName);

    if (!node && writeFlag)
    {
        node = gRoot->bind(cFileName, 0);
    }
    if (node)
    {
        es::File* file;
        file = reinterpret_cast<es::File*>(node->queryInterface(es::File::iid()));
        if (!file)
        {
            f->file = 0;
        }
        else
        {
            f->file = file->getStream();
            f->writable = writeFlag ? true : false;
            file->release();
        }
        node->release();
    }
    else
    {
        f->file = 0;
    }

    if (!f->file)
    {
        f->sessionID = 0;
        f->fileSize = 0;
        FPRINTF("%s: failed.\n", __func__);
        return interpreterProxy->success(false);
    }
    else
    {
        f->sessionID = thisSession;
        // compute and cache file size
        es::Stream* stream = getStream(f);
        long long size;
        size = stream->getSize();
        f->fileSize = size;
    }
    f->lastOp = UNCOMMITTED;
}

/* Read count bytes from the given file into byteArray starting at
   startIndex. byteArray is the address of the first byte of a
   Squeak bytes object (e.g. String or ByteArray). startIndex
   is a zero-based index; that is a startIndex of 0 starts writing
   at the first byte of byteArray.
 */
size_t sqFileReadIntoAt(SQFile* f, size_t count, int byteArrayIndex, size_t startIndex)
{
    FPRINTF("%s(%p)\n", __func__, f);

    char* dst;
    int bytesRead;

    if (!sqFileValid(f))
    {
        FPRINTF("%s(%p) : failed.\n", __func__, f);
        return interpreterProxy->success(false);
    }

    dst = (char*) (byteArrayIndex + startIndex);
    es::Stream* stream = getStream(f);
    bytesRead = stream->read(dst, count);
    f->lastOp = READ_OP;
    return bytesRead;
}

int sqFileRenameOldSizeNewSize(int oldNameIndex, int oldNameSize, int newNameIndex, int newNameSize)
{
    FPRINTF("%s()\n", __func__);

    char cOldName[1000], cNewName[1000];

    if (sizeof(cOldName) <= oldNameSize || sizeof(cNewName) <= newNameSize)
    {
        FPRINTF("%s() : failed.\n", __func__);
        return interpreterProxy->success(false);
    }

    // copy the file names into null-terminated C strings
    sqFilenameFromString(cOldName, oldNameIndex, oldNameSize);
    sqFilenameFromString(cNewName, newNameIndex, newNameSize);

    if (gRoot->rename(cOldName, cNewName) < 0)
    {
        FPRINTF("%s() : failed.\n", __func__);
        return interpreterProxy->success(false);
    }
    return 1;
}

// Set the file's read/write head to the given position.
int sqFileSetPosition(SQFile* f, squeakFileOffsetType position)
{
    FPRINTF("%s(%p, %d)\n", __func__, f, position);

    if (!sqFileValid(f))
    {
        FPRINTF("%s(%p) : failed.\n", __func__, f);
        return interpreterProxy->success(false);
    }

    es::Stream* stream = getStream(f);
    stream->setPosition(position);
    f->lastOp = UNCOMMITTED;
    return 1;
}

// Return the length of the given file.
squeakFileOffsetType sqFileSize(SQFile* f)
{
    FPRINTF("%s(%p) :", __func__, f);

    if (!sqFileValid(f))
    {
        FPRINTF(" failed.\n");
        return interpreterProxy->success(false);
    }

    FPRINTF(" %d.\n", f->fileSize);
    return (squeakFileOffsetType) f->fileSize;
}

int sqFileValid(SQFile* f)
{
    return (f != NULL) && (f->file != NULL) && (f->sessionID == thisSession);
}

/* Write count bytes to the given writable file starting at startIndex
   in the given byteArray. (See comment in sqFileReadIntoAt for interpretation
   of byteArray and startIndex).
 */
size_t sqFileWriteFromAt(SQFile *f, size_t count, int byteArrayIndex, size_t startIndex)
{
    FPRINTF("%s(%p, %d) : %d %d\n", __func__, f, count, sqFileValid(f), f->writable);

    char* src;
    int bytesWritten;

    if (!(sqFileValid(f) && f->writable))
    {
        FPRINTF("%s: failed.\n", __func__);
        return interpreterProxy->success(false);
    }

    src = (char*) (byteArrayIndex + startIndex);
    es::Stream* stream = getStream(f);
    bytesWritten = stream->write(src, count);

    // update file size
    long long size;
    size = stream->getSize();
    f->fileSize = size;

    if (bytesWritten != count)
    {
        FPRINTF("%s: failed.\n", __func__);
        interpreterProxy->success(false);
    }
    f->lastOp = WRITE_OP;
    return bytesWritten;
}

int sqFileFlush(SQFile* f)
{
    if (!sqFileValid(f))
    {
        FPRINTF("%s: failed.\n", __func__);
        return interpreterProxy->success(false);
    }
    es::Stream* stream = getStream(f);
    stream->flush();
    return 1;
}

int sqFileTruncate(SQFile* f, squeakFileOffsetType offset)
{
    FPRINTF("%s(%p)\n", __func__, f);

    if (!sqFileValid(f))
    {
        FPRINTF("%s: failed.\n", __func__);
        return interpreterProxy->success(false);
    }
    es::Stream* stream = getStream(f);
    stream->setSize(offset);

    // update file size
    long long size;
    size = stream->getSize();
    f->fileSize = size;

    stream->setPosition(size);
    f->lastOp = UNCOMMITTED;

    return 1;
}

int sqFileThisSession(void)
{
    return thisSession;
}
