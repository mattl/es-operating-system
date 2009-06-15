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

// ISO 9660 File Subsystem

#include <es.h>
#include <es/dateTime.h>
#include <es/endian.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/types.h>
#include <es/utf.h>
#include <es/base/ICache.h>
#include <es/base/IFile.h>
#include <es/base/IStream.h>
#include <es/device/IDiskManagement.h>
#include <es/device/IIso9660FileSystem.h>
#include <es/util/IIterator.h>
#include <es/naming/IBinding.h>
#include <es/naming/IContext.h>
#include "iso9660.h"

class Iso9660FileSystem;
class Iso9660Iterator;
class Iso9660Stream;
class Iso9660StreamUcs2;

class Iso9660Stream : public es::Stream, public es::Context, public es::Binding, public es::File
{
    friend class Iso9660FileSystem;
    friend class Iso9660Iterator;
    friend class Iso9660StreamUcs2;

    Iso9660FileSystem*  fileSystem;
    Link<Iso9660Stream> link;

    Ref                 ref;
    es::Cache*             cache;
    Iso9660Stream*      parent;
    u32                 dirLocation;
    u32                 offset;
    u32                 location;
    u32                 size;
    u8                  flags;
    DateTime            dateTime;

public:
    Iso9660Stream(Iso9660FileSystem* fileSystem, Iso9660Stream* parent, u32 offset, u8* record);
    ~Iso9660Stream();

    bool isRoot();
    int hashCode() const;

    bool findNext(es::Stream* dir, u8* record);
    virtual Iso9660Stream* lookupPathName(const char*& name);

    // IFile
    unsigned int getAttributes();
    long long getCreationTime();
    long long getLastAccessTime();
    long long getLastWriteTime();
    void setAttributes(unsigned int attributes);
    void setCreationTime(long long time);
    void setLastAccessTime(long long time);
    void setLastWriteTime(long long time);
    bool canRead();
    bool canWrite();
    bool isDirectory();
    bool isFile();
    bool isHidden();
    es::Stream* getStream();
    es::Pageable* getPageable();

    // IStream
    long long getPosition();
    void setPosition(long long pos);
    long long getSize();
    void setSize(long long size);
    int read(void* dst, int count);
    int read(void* dst, int count, long long offset);
    int write(const void* src, int count);
    int write(const void* src, int count, long long offset);
    void flush();

    // IBinding
    Object* getObject();
    void setObject(Object* object);
    const char* getName(void* name, int len);

    // IContext
    es::Binding* bind(const char* name, Object* object);
    es::Context* createSubcontext(const char* name);
    int destroySubcontext(const char* name);
    Object* lookup(const char* name);
    int rename(const char* oldName, const char* newName);
    int unbind(const char* name);
    es::Iterator* list(const char* name);

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};

class Iso9660StreamUcs2 : public Iso9660Stream
{
    static const u16 dot[2];
    static const u16 dotdot[3];
public:
    Iso9660StreamUcs2(Iso9660FileSystem* fileSystem, Iso9660Stream* parent, u32 offset, u8* record) :
        Iso9660Stream(fileSystem, parent, offset, record)
    {
    }
    Iso9660Stream* lookupPathName(const char*& name);
    const char* getName(char* name, int len);
};

class Iso9660FileSystem : public es::Iso9660FileSystem
{
    typedef List<Iso9660Stream, &Iso9660Stream::link> Iso9660StreamChain;
    friend class Iso9660Stream;

    Ref                 ref;
    es::Stream*            disk;
    es::Cache*             diskCache;
    Iso9660Stream*      root;
    const char*         escapeSequences;
    u8                  rootRecord[256];

    size_t              hashSize;
    Iso9660StreamChain* hashTable;

    u16                 bytsPerSec;

public:
    static const DateTime epoch;
    static const char* ucs2EscapeSequences[3];

    Iso9660FileSystem();
    Iso9660FileSystem(es::Stream* disk);
    ~Iso9660FileSystem();

    void init();

    Iso9660Stream* lookup(u32 dirLocation, u32 offset);
    void add(Iso9660Stream* stream);
    void remove(Iso9660Stream* stream);

    Iso9660Stream* getRoot();
    Iso9660Stream* createStream(Iso9660FileSystem* fileSystem, Iso9660Stream* parent, u32 offset, u8* record);

    static int hashCode(u32 dirLocation, u32 offset);
    static DateTime getTime(u8* dt);
    static const char* splitPath(const char* path, char* file);
    static const char* splitPath(const char* path, u16* file);
    static bool isDelimitor(int c);

    // IFileSystem
    void mount(es::Stream* disk);
    void dismount();
    void getRoot(es::Context** root);
    long long getFreeSpace();
    long long getTotalSpace();
    int checkDisk(bool fixError);
    void format();
    int defrag();

    // IInterface
    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();

    // [Constructor]
    class Constructor : public es::Iso9660FileSystem::Constructor
    {
    public:
        es::Iso9660FileSystem* createInstance();
        Object* queryInterface(const char* riid);
        unsigned int addRef();
        unsigned int release();
    };

    static void initializeConstructor();
};

class Iso9660Iterator : public es::Iterator
{
    Ref             ref;
    Iso9660Stream*  stream;
    long long       ipos;

public:

    Iso9660Iterator(Iso9660Stream* stream);
    ~Iso9660Iterator();

    bool hasNext();
    Object* next();
    int remove();

    Object* queryInterface(const char* riid);
    unsigned int addRef();
    unsigned int release();
};
