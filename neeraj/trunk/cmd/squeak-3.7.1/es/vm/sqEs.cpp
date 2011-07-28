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
#include <es/handle.h>
#include <es/md5.h>
#include <es/usage.h>
#include <es/base/IPageable.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <es/base/IService.h>
#include <es/device/IAudioFormat.h>
#include <es/device/IBeep.h>
#include <es/device/ICursor.h>
#include <es/device/IRtc.h>
#include <es/naming/IContext.h>
#include "../IEventQueue.h"



#ifndef _DEBUG
#define FPRINTF(...)    (__VA_ARGS__)
#else
#define FPRINTF(...)    esReport(__VA_ARGS__)
#endif

es::CurrentProcess* System();

extern "C"
{
    #include "sq.h"

    void* sqAllocateMemory(int minHeapSize, int desiredHeapSize);

    void* os_exports[][3] =
    {
        { NULL, NULL, NULL }
    };
}

extern void initInputProcess();

extern void* audioProcess(void* param);
extern void* recordProcess(void* param);
extern void* networkProcess(void* param);

/*** Constants ***/
static const DateTime Epoch(1901, 1, 1);

/*** Variables ***/
Handle<es::Context> gRoot;

/*** Variables -- image and path names ***/
#define IMAGE_NAME_SIZE 300
char imageName[IMAGE_NAME_SIZE + 1];  /* full path to image */

#define SHORTIMAGE_NAME_SIZE 100
char shortImageName[SHORTIMAGE_NAME_SIZE + 1];  /* just the image file name */

#define VMPATH_SIZE 300
char vmPath[VMPATH_SIZE + 1];  /* full path to interpreter's directory */

/*** I/O Primitives ***/

static es::Beep* gIbeep;
int (ioMSecs)(void);

int ioBeep(void)
{
    /* optional; could be noop. play a beep through the speaker. */
    if (gIbeep)
    {
        gIbeep->beep();
    }
    return 0;
}

int ioExit(void)
{
    Handle<es::Context> root = System()->getRoot();
    Handle<es::Cursor> cursor = root->lookup("device/cursor");
    cursor->hide();

    Handle<es::Service> console = root->lookup("/device/console");
    if (console)
    {
        console->start();
    }

    /* optional; could be noop. exit from the Squeak application. */
    System()->exit(0);
}

int (ioLowResMSecs)(void)
{
/* Note: The Squeak VM uses three different clocks functions for
   timing. The primary one, ioMSecs(), is used to implement Delay
   and Time millisecondClockValue. The resolution of this clock
   determines the resolution of these basic timing functions. For
   doing real-time control of music and MIDI, a clock with resolution
   down to one millisecond is preferred, but a coarser clock (say,
   1/60th second) can be used in a pinch. The VM calls a different
   clock function, ioLowResMSecs(), in order to detect long-running
   primitives. This function must be inexpensive to call because when
   a Delay is active it is polled twice per primitive call. On several
   platforms (Mac, Win32), the high-resolution system clock used in
   ioMSecs() would incur enough overhead in this case to slow down the
   the VM significantly. Thus, a cheaper clock with low resolution is
   used to implement ioLowResMSecs() on these platforms. Finally, the
   function ioMicroMSecs() is used only to collect timing statistics
   for the garbage collector and other VM facilities. (The function
   name is meant to suggest that the function is based on a clock
   with microsecond accuracy, even though the times it returns are
   in units of milliseconds.) This clock must have enough precision to
   provide accurate timings, and normally isn't called frequently
   enough to slow down the VM. Thus, it can use a more expensive clock
   that ioMSecs(). By default, all three clock functions are defined
   here as macros based on the standard C library function clock().
   Any of these macros can be overridden in sqPlatformSpecific.h.
*/
    return ioMicroMSecs();
}

int (ioMicroMSecs)(void)
{
    /* millisecond clock based on microsecond timer (about 60 times slower than clock()!!) */
    /* Note: This function and ioMSecs() both return a time in milliseconds. The difference
       is that ioMicroMSecs() is called only when precise millisecond resolution is essential,
       and thus it can use a more expensive timer than ioMSecs, which is called frequently.
       However, later VM optimizations reduced the frequency of calls to ioMSecs to the point
       where clock performance became less critical, and we also started to want millisecond-
       resolution timers for real time applications such as music. Thus, on the Mac, we've
       opted to use the microsecond clock for both ioMSecs() and ioMicroMSecs(). */
    long long ticks = System()->getNow();
    /* Make sure the value fits into Squeak SmallIntegers */
    return (int) (ticks / 10000) & 0x3FFFFFFF;
}

int (ioMSecs)(void)
{
    /* return a time in milliseconds for use in Delays and Time millisecondClockValue */
    /* Note: This was once a macro based on clock(); it now uses the microsecond clock for
       greater resolution. See the comment in ioMicroMSecs(). */
    long long ticks = System()->getNow();
    /* Make sure the value fits into Squeak SmallIntegers */
    return (int) (ticks / 10000) & 0x3FFFFFFF;
}

int ioSeconds(void)
{
    /* return the time in seconds since midnight of Jan 1, 1901.  */
    /* optional: could simply return 0.  */
    long long ticks = System()->getNow();
    return (int) ((ticks - Epoch.getTicks()) / 10000000);
}

/*** VM Home Directory Path ***/

int reserveExtraCHeapBytes(int origHeapSize, int bytesToReserve);

int reserveExtraCHeapBytes(int origHeapSize, int bytesToReserve)
{
    return origHeapSize + bytesToReserve;
}

int vmPathSize(void)
{
    /* return the length of the path string for the directory containing the VM. */
    return strlen(vmPath);
}

int vmPathGetLength(int sqVMPathIndex, int length)
{
    /* copy the path string for the directory containing the VM into the given Squeak string. */
    char *stVMPath = (char *) sqVMPathIndex;
    int count, i;

    count = strlen(vmPath);
    count = (length < count) ? length : count;

    /* copy the file name into the Squeak string */
    for (i = 0; i < count; i++) {
        stVMPath[i] = vmPath[i];
    }
    return count;
}

/*** Image File Name ***/

int imageNameSize(void)
{
    /* return the length of the Squeak image name. */
    return strlen(imageName);
}

int imageNameGetLength(int sqImageNameIndex, int length)
{
    /* copy the Squeak image name into the given Squeak string. */
    char *sqImageName = (char *) sqImageNameIndex;
    int count, i;

    count = strlen(imageName);
    count = (length < count) ? length : count;

    /* copy the file name into the Squeak string */
    for (i = 0; i < count; i++) {
        sqImageName[i] = imageName[i];
    }
    return count;
}

int imageNamePutLength(int sqImageNameIndex, int length)
{
    /* copy from the given Squeak string into the imageName variable. */
    char *sqImageName = (char *) sqImageNameIndex;
    int count, i, ch, j;
    int lastColonIndex = -1;

    count = (IMAGE_NAME_SIZE < length) ? IMAGE_NAME_SIZE : length;

    /* copy the file name into a null-terminated C string */
    for (i = 0; i < count; i++) {
        ch = imageName[i] = sqImageName[i];
        if (ch == ':') {
            lastColonIndex = i;
        }
    }
    imageName[count] = 0;

    /* copy short image name into a null-terminated C string */
    for (i = lastColonIndex + 1, j = 0; i < count; i++, j++) {
        shortImageName[j] = imageName[i];
    }
    shortImageName[j] = 0;

    return count;
}

/*** Clipboard Support ***/

int clipboardReadIntoAt(int count, int byteArrayIndex, int startIndex) {
    /* return number of bytes read from clipboard; stubbed out. */
    return 0;
}

int clipboardSize(void) {
    /* return the number of bytes of data the clipboard; stubbed out. */
    return 0;
}

int clipboardWriteFromAt(int count, int byteArrayIndex, int startIndex) {
    /* write count bytes to the clipboard; stubbed out. */
    return 0;
}

/*** System Attributes ***/

char * GetAttributeString(int id) {
    /* This is a hook for getting various status strings back from
       the OS. In particular, it allows Squeak to be passed arguments
       such as the name of a file to be processed. Command line options
       are reported this way as well, on platforms that support them.
    */

    // id #0 should return the full name of VM; for now it just returns its path
    if (id == 0) return vmPath;
    // id #1 should return imageName, but returns empty string in this release to
    // ease the transition (1.3x images otherwise try to read image as a document)
    if (id == 1) return "";  /* will be imageName */
    if (id == 2) return "";

    /* the following attributes describe the underlying platform: */
    if (id == 1001) return "ES";
    if (id == 1002) return "Nintendo ES";
    if (id == 1003) return "x86";

    /* attribute undefined by this platform */
    success(false);
    return "";
}

int attributeSize(int id) {
    /* return the length of the given attribute string. */
    return strlen(GetAttributeString(id));
}

int getAttributeIntoLength(int id, int byteArrayIndex, int length) {
    /* copy the attribute with the given id into a Squeak string. */
    char *srcPtr, *dstPtr, *end;
    int charsToMove;

    srcPtr = GetAttributeString(id);
    charsToMove = strlen(srcPtr);
    if (charsToMove > length) {
        charsToMove = length;
    }

    dstPtr = (char *) byteArrayIndex;
    end = srcPtr + charsToMove;
    while (srcPtr < end) {
        *dstPtr++ = *srcPtr++;
    }
    return charsToMove;
}

/*** Profiling Stubs ***/

int clearProfile(void)
{
    success(false);
}

int dumpProfile(void)
{
    success(false);
}

int startProfiling(void)
{
    success(false);
}

int stopProfiling(void)
{
    success(false);
}

/*** External Primitive Support (No-ops) ***/

/* The next three functions must be implemented by sqXYZExternalPrims.c */
/* ioLoadModule:
    Load a module from disk.
    WARNING: this always loads a *new* module. Don't even attempt to find a loaded one.
    WARNING: never primitiveFail() within, just return 0
*/
int ioLoadModule(char *pluginName)
{
    esReport("ioLoadModule: %s\n", pluginName);
    return 0;
}

/* ioFindExternalFunctionIn:
    Find the function with the given name in the moduleHandle.
    WARNING: never primitiveFail() within, just return 0.
*/
int ioFindExternalFunctionIn(char *lookupName, int moduleHandle)
{
    esReport("ioFindExternalFunctionIn\n");
    return 0;
}

/* ioFreeModule:
    Free the module with the associated handle.
    WARNING: never primitiveFail() within, just return 0.
*/
int ioFreeModule(int moduleHandle)
{
    esReport("ioFreeModule: %d\n", moduleHandle);
    return 0;
}

/* Power Management */

int ioDisablePowerManager(int disableIfNonZero)
{
//    esReport("ioDisablePowerManager: %d\n", disableIfNonZero);

}

/*** Image File Read/Write ***/

// #define USE_MMAP

es::File* imageFile;

void* sqAllocateMemory(int minHeapSize, int desiredHeapSize)
{
#ifndef USE_MMAP
    return new(std::nothrow) u8[desiredHeapSize];
#else
    // Map imageFile.
    es::Pageable* pageable = imageFile->getPageable();
    void* addr = System()->map(0, desiredHeapSize,
                               es::CurrentProcess::PROT_READ | es::CurrentProcess::PROT_WRITE,
                               es::CurrentProcess::MAP_PRIVATE,
                               pageable, 0);
    pageable->release();
    imageFile->release();
    imageFile = 0;
    return (void*) ((u8*) addr + 64);   // XXX hack. 64 is the offset after the header.
#endif
}

int sqImageFileClose(sqImageFile f)
{
    FPRINTF("%s(%p);\n", __func__, f);
    es::Stream* stream = (es::Stream*) f;
    stream->release();
    return 0;
}

sqImageFile sqImageFileOpen(char *fileName, char *mode)
{
    FPRINTF("%s(\"%s\", \"%s\");\n", __func__, fileName, mode);
    Object* interface = gRoot->lookup(fileName);
    imageFile = reinterpret_cast<es::File*>(interface->queryInterface(es::File::iid()));
    interface->release();
    es::Stream* stream = imageFile->getStream();
    return (sqImageFile) stream;
}

squeakFileOffsetType sqImageFilePosition(sqImageFile f)
{
    FPRINTF("%s(%p) :", __func__, f);
    es::Stream* stream = (es::Stream*) f;
    long long pos;
    pos = stream->getPosition();
    FPRINTF("%lld", pos);
    return pos;
}

size_t sqImageFileRead(void *ptr, size_t elementSize, size_t count, sqImageFile f)
{
    FPRINTF("%s(%d, %d) :", __func__, elementSize, count);
#ifdef USE_MMAP
    if (4 < elementSize * count)
    {
        // Use mmap
        return elementSize * count;
    }
#endif
    es::Stream* stream = (es::Stream*) f;
    size_t len = stream->read(ptr, elementSize * count);
    FPRINTF(" %d\n", len);
    return (0 < len) ? len / elementSize : len;
}

squeakFileOffsetType sqImageFileSeek(sqImageFile f, squeakFileOffsetType pos)
{
    FPRINTF("%s(%lld)\n", __func__, pos);
    es::Stream* stream = (es::Stream*) f;
    stream->setPosition(pos);
    return pos;
}

size_t sqImageFileWrite(void *ptr, size_t elementSize, size_t count, sqImageFile f)
{
    FPRINTF("%s(%d, %d) : ", __func__, elementSize, count);
    es::Stream* stream = (es::Stream*) f;
    size_t len = stream->write(ptr, elementSize * count);
    FPRINTF("%d\n", len);
    return (0 < len) ? len / elementSize : len;
}

int main()
{
    esReport("Squeak3.7\n");

    Handle<es::Context> root = System()->getRoot();

    Handle<es::Service> console = root->lookup("/device/console");
    if (console)
    {
        console->stop();
    }

    Handle<es::Beep> beep = root->lookup("device/beep");

    gIbeep = beep;

    imageName[0] = shortImageName[0] = vmPath[0] = 0;
    strcpy(imageName, "Squeak3.7-5989-full.image");
    strcpy(shortImageName, "Squeak3.7-5989-full.image");

    gRoot = root->lookup("file");
#if 1
    Handle<es::Iterator> iter = gRoot->list("");
    while (iter->hasNext())
    {
        char name[1024];
        Handle<es::Binding> binding(iter->next());
        binding->getName(name, sizeof name);
        esReport("'%s'\n", name);
    }
#endif

    sqImageFile f = sqImageFileOpen(imageName, "rb");
    if (f == 0)
    {
        /* give a Mac-specific error message if image file is not found */
        esReport("Could not open the Squeak image file '%s'\n\n", imageName);
        esReport("In this minimal VM, the image file must be named 'squeak.image'\n");
        esReport("and must be in the same directory as the Squeak application.\n");
        esReport("Press the return key to exit.\n");
        esReport("Aborting...\n");
        return 1;
    }
    readImageFromFileHeapSize(f, 64 * 1024 * 1024);
    sqImageFileClose(f);

    esReport("Loaded image file\n");

    initInputProcess();

    es::Thread* audioThread = System()->createThread(reinterpret_cast<void*>(audioProcess), 0);
    audioThread->setPriority(es::Thread::Normal + 1);
    audioThread->start();

    es::Thread* recordThread = System()->createThread(reinterpret_cast<void*>(recordProcess), 0);
    recordThread->setPriority(es::Thread::Normal + 1);
    recordThread->start();

    es::Thread* networkThread = System()->createThread(reinterpret_cast<void*>(networkProcess), 0);
    networkThread->setPriority(es::Thread::Normal + 2);
    networkThread->start();

    // Run Squeak
    interpret();
}
