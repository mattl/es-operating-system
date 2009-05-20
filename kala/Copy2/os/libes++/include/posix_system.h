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

#ifndef GOOGLE_ES_LIBES_POSIX_SYSTEM_H_INCLUDED
#define GOOGLE_ES_LIBES_POSIX_SYSTEM_H_INCLUDED

#include <map>

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <wchar.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <es/broker.h>
#include <es/dateTime.h>
#include <es/handle.h>
#include <es/interlocked.h>
#include <es/objectTable.h>
#include <es/ref.h>
#include <es/reflect.h>
#include <es/timeSpan.h>
#include <es/base/IProcess.h>

#include <sys/mman.h>

namespace es
{

namespace posix
{

class Stream : public es::Stream, public es::Pageable
{
    Ref ref;
    int fd;

protected:
    void setfd(int fd)
    {
        this->fd = fd;
    }

public:
    Stream(int fd) :
        fd(fd)
    {
    }

    ~Stream()
    {
        if (3 <= fd)
        {
            close(fd);
        }
    }

    int getfd()
    {
        return fd;
    }

    //
    // IStream
    //

    long long getPosition()
    {
        return lseek(fd, 0, SEEK_CUR);
    }

    void setPosition(long long pos)
    {
        lseek(fd, pos, SEEK_SET);
    }

    long long getSize()
    {
        struct stat buf;

        fstat(fd, &buf);
        return buf.st_size;
    }

    void setSize(long long size)
    {
        ftruncate(fd, size);
    }

    int read(void* buffer, int size)
    {
        return ::read(fd, buffer, size);
    }

    int read(void* buffer, int size, long long offset)
    {
        setPosition(offset);
        return ::read(fd, buffer, size);
    }

    int write(const void* buffer, int size)
    {
        return ::write(fd, buffer, size);
    }

    int write(const void* buffer, int size, long long offset)
    {
        setPosition(offset);
        return ::write(fd, buffer, size);
    }

    void flush()
    {
        fsync(fd);
    }

    //
    // IPageable - only for mmap support
    //

    unsigned long long get(long long offset)
    {
        return 0;
    }

    void put(long long offset, unsigned long long pte)
    {
    }

    //
    // IInterface
    //

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::Stream::iid()) == 0)
        {
            objectPtr = static_cast<es::Stream*>(this);
        }
        else if (strcmp(riid, es::Pageable::iid()) == 0)
        {
            objectPtr = static_cast<es::Pageable*>(this);
        }
        else if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::Stream*>(this);
        }
        else
        {
            return NULL;
        }
        objectPtr->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

class File : public es::File, public es::Binding
{
protected:
    Ref     ref;
    int     fd;
    char*   name;

public:
    File(int fd, const char* name) :
        fd(fd)
    {
        const char* slash = strrchr(name, '/');
        if (slash)
        {
            this->name = strdup(++slash);
        }
        else
        {
            this->name = strdup(name);
        }
    }

    ~File()
    {
        if (name)
        {
            free(name);
        }
        if (0 <= fd)
        {
            close(fd);
        }
    }

    int getfd()
    {
        return fd;
    }

    // IFile
    unsigned int getAttributes()
    {
    }
    long long getCreationTime()
    {
    }
    long long getLastAccessTime()
    {
    }
    long long getLastWriteTime()
    {
    }
    void setAttributes(unsigned int attributes)
    {
    }
    void setCreationTime(long long time)
    {
    }
    void setLastAccessTime(long long time)
    {
    }
    void setLastWriteTime(long long time)
    {
    }
    bool canRead()
    {
    }
    bool canWrite()
    {
    }
    bool isDirectory()
    {
    }
    bool isFile()
    {
    }
    bool isHidden()
    {
    }

    long long getSize()
    {
    }

    es::Stream* getStream()
    {
        return new Stream(dup(fd));
    }

    es::Pageable* getPageable()
    {
        return new Stream(dup(fd));
    }

    //
    // IBinding
    //
    Object* getObject()
    {
        addRef();
        return static_cast<es::File*>(this);
    }

    void setObject(Object* object)
    {
        esThrow(EACCES); // [check] appropriate?
    }

    const char* getName(void* name, int len)
    {
        if (len < 1)
        {
            return 0;
        }
        const char* src = this->name;
        char* dst = static_cast<char*>(name);
        for (int i = 0; i < len; ++i)
        {
            char c = *dst++ = *src++;
            if (c == '\0')
            {
                return static_cast<char*>(name);
            }
        }
        static_cast<char*>(name)[len - 1] = '\0';
        return static_cast<char*>(name);
    }

    //
    // IInterface
    //

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::File::iid()) == 0)
        {
            objectPtr = static_cast<es::File*>(this);
        }
        else if (strcmp(riid, es::Binding::iid()) == 0)
        {
            objectPtr = static_cast<es::Binding*>(this);
        }
        else if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::File*>(this);
        }
        else
        {
            return NULL;
        }
        objectPtr->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

class Iterator : public es::Iterator
{
    Ref     ref;
    DIR*    dir;
    off_t   offset;

public:
    Iterator(int fd) :
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 4)
        dir(fdopendir(dup(fd))),
#else
        dir(0),
#endif
        offset(-1)
    {
        if (dir)
        {
            rewinddir(dir);
        }
    }

    int getfd()
    {
        return dirfd(dir);
    }

    ~Iterator()
    {
        if (dir)
        {
            closedir(dir);
        }
    }

    bool hasNext()
    {
        off_t tmp = telldir(dir);
        if (tmp == -1)
        {
            return false;
        }
        bool result = readdir(dir) ? true : false;
        seekdir(dir, tmp);
        return result;
    }

    Object* next();

    int remove()
    {
        if (offset == -1)
        {
            return -1;
        }
        seekdir(dir, offset);
        if (struct dirent* ent = readdir(dir))
        {
            seekdir(dir, offset);
            offset = -1;
            return ::remove(ent->d_name);
        }
        return -1;
    }

    //
    // IInterface
    //

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::Iterator::iid()) == 0)
        {
            objectPtr = static_cast<es::Iterator*>(this);
        }
        else if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::Iterator*>(this);
        }
        else
        {
            return NULL;
        }
        objectPtr->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

class Dir : public es::Context, public File
{
public:
    Dir(int fd, const char* name) :
        File(fd, name)
    {
    }

    ~Dir()
    {
    }

    //
    // IFile
    //

    es::Stream* getStream()
    {
        return 0;
    }

    //
    // IContext
    //

    es::Binding* bind(const char* name, Object* element)
    {
        if (fchdir(fd) == -1)
        {
            return 0;
        }
        int fd = open(name, O_CREAT | O_RDWR | O_TRUNC, 0777);
        if (fd == -1)
        {
            return 0;
        }

        return new File(fd, name);
    }

    es::Context* createSubcontext(const char* name)
    {
        if (fchdir(fd) == -1)
        {
            return 0;
        }
        if (mkdir(name, 0777) == -1)
        {
            return 0;
        }
        return new Dir(open(name, O_RDONLY), name);
    }

    int destroySubcontext(const char* name)
    {
        if (fchdir(fd) == -1)
        {
            return -1;
        }
        return rmdir(name);
    }

    Object* lookup(const char* name)
    {
        if (fchdir(fd) == -1)
        {
            return 0;
        }


        int newfd;
#if 0   // XXX for fexecve()
        newfd = open(name, O_RDWR);
        if (newfd == -1)
#endif
        {
            newfd = open(name, O_RDONLY);
        }
        if (newfd == -1)
        {
            return 0;
        }

        struct stat st;
        if (fstat(newfd, &st) == -1)
        {
            close(newfd);
            return 0;
        }
        if (S_ISDIR(st.st_mode))
        {
            return static_cast<es::File*>(new Dir(newfd, name));
        }
        else
        {
            return static_cast<es::File*>(new File(newfd, name));
        }
    }

    int rename(const char* oldName, const char* newName)
    {
        if (fchdir(fd) == -1)
        {
            return -1;
        }
        return rename(oldName, newName);
    }

    int unbind(const char* name)
    {
        if (fchdir(fd) == -1)
        {
            return -1;
        }
        return unlink(name);
    }

    es::Iterator* list(const char* name)
    {
        if (*name == '\0')
        {
            return new Iterator(fd);
        }

        if (fchdir(fd) == -1)
        {
            return 0;
        }

        int newfd = open(name, O_RDWR);
        if (newfd == -1)
        {
            newfd = open(name, O_RDONLY);
        }
        if (newfd == -1)
        {
            return 0;
        }

        struct stat st;
        if (fstat(newfd, &st) == -1)
        {
            close(newfd);
            return 0;
        }
        if (S_ISDIR(st.st_mode))
        {
            return new Iterator(newfd);
        }
        else
        {
            close(newfd);
            return 0;
        }
    }

    //
    // IInterface
    //

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::File::iid()) == 0)
        {
            objectPtr = static_cast<es::File*>(this);
        }
        else if (strcmp(riid, es::Binding::iid()) == 0)
        {
            objectPtr = static_cast<es::Binding*>(this);
        }
        else if (strcmp(riid, es::Context::iid()) == 0)
        {
            objectPtr = static_cast<es::Context*>(this);
        }
        else if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::Context*>(this);
        }
        else
        {
            return NULL;
        }
        objectPtr->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

inline Object* Iterator::next()
{
    if (fchdir(dirfd(dir)) == -1)
    {
        return 0;
    }

    offset = telldir(dir);
    while (struct dirent* ent = readdir(dir))
    {
        int newfd = open(ent->d_name, O_RDWR);
        if (newfd == -1)
        {
            newfd = open(ent->d_name, O_RDONLY);
        }
        if (newfd == -1)
        {
            continue;
        }

        struct stat st;
        if (fstat(newfd, &st) == -1)
        {
            close(newfd);
            continue;
        }
        if (S_ISDIR(st.st_mode))
        {
            return static_cast<es::File*>(new Dir(newfd, ent->d_name));
        }
        else
        {
            return static_cast<es::File*>(new File(newfd, ent->d_name));
        }
    }
    offset = -1;
    return 0;
}

}   // namespace posix

}   // namespace es

#endif // GOOGLE_ES_LIBES_POSIX_SYSTEM_H_INCLUDED
