/*
 * Copyright (c) 2007
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <es/dateTime.h>
#include <es/handle.h>
#include <es/timeSpan.h>
#include <es/ref.h>
#include <es/base/IProcess.h>
#include <es/classFactory.h>
#include <es/clsid.h>

int esInit(IInterface** nameSpace);

namespace
{

class Stream : public IStream
{
    Ref     ref;
    FILE*   file;

public:
    Stream(FILE* file) :
        file(file)
    {
    }

    ~Stream()
    {
        if (file && !isatty(fileno(file)))
        {
            fclose(file);
        }
    }

    //
    // IStream
    //

    long long getPosition()
    {
        return ftell(file);
    }

    void setPosition(long long pos)
    {
        fseek(file, pos, SEEK_SET);
    }

    long long getSize()
    {
        long tmp = ftell(file);
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, tmp, SEEK_SET);
        return size;
    }

    void setSize(long long size)
    {
        ftruncate(fileno(file), size);
    }

    int read(void* buffer, int size)
    {
        return fread(buffer, 1, size, file);
    }

    int read(void* buffer, int size, long long offset)
    {
        setPosition(offset);
        return fread(buffer, 1, size, file);
    }

    int write(const void* buffer, int size)
    {
        return fwrite(buffer, 1, size, file);
    }

    int write(const void* buffer, int size, long long offset)
    {
        setPosition(offset);
        return fwrite(buffer, 1, size, file);
    }

    void flush()
    {
        fflush(file);
    }

    //
    // IInterface
    //

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IStream)
        {
            *objectPtr = static_cast<IStream*>(this);
        }
        else if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IStream*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
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

class File : public IFile, public IBinding
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
    int getAttributes(unsigned int& attributes)
    {
    }
    int getCreationTime(long long& time)
    {
    }
    int getLastAccessTime(long long& time)
    {
    }
    int getLastWriteTime(long long& time)
    {
    }
    int setAttributes(unsigned int attributes)
    {
    }
    int setCreationTime(long long time)
    {
    }
    int setLastAccessTime(long long time)
    {
    }
    int setLastWriteTime(long long time)
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

    IStream* getStream()
    {
        FILE* file = fdopen(fd, "rw");
        if (!file)
        {
            file = fdopen(fd, "r");
        }
        if (!file)
        {
            return 0;
        }
        fd = dup(fd);
        return new Stream(file);
    }

    IPageable* getPageable()
    {
        return 0;
    }

    //
    // IBinding
    //
    IInterface* getObject()
    {
        addRef();
        return static_cast<IFile*>(this);
    }

    int setObject(IInterface* object)
    {
        return -1;
    }

    int getName(char* name, unsigned int len)
    {
        const char* p = this->name;
        int i;
        for (i = 0; i < len; ++i)
        {
            char c = *name++ = *p++;
            if (c == '\0')
            {
                break;
            }
        }
        return i;
    }

    //
    // IInterface
    //

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IFile)
        {
            *objectPtr = static_cast<IFile*>(this);
        }
        else if (riid == IID_IBinding)
        {
            *objectPtr = static_cast<IBinding*>(this);
        }
        else if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IFile*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
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

class Iterator : public IIterator
{
    Ref     ref;
    DIR*    dir;
    off_t   offset;

public:
    Iterator(int fd) :
        dir(fdopendir(dup(fd))),
        offset(-1)
    {
        if (dir)
        {
            rewinddir(dir);
        }
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

    IInterface* next();

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

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IIterator)
        {
            *objectPtr = static_cast<IIterator*>(this);
        }
        else if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IIterator*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
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

class Dir : public IContext, public File
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

    IStream* getStream()
    {
        return 0;
    }

    //
    // IContext
    //

    IBinding* bind(const char* name, IInterface* element)
    {
        if (fchdir(fd) == -1)
        {
            return 0;
        }
        int fd = open(name, O_CREAT | O_RDWR, 0777);
        if (fd == -1)
        {
            return 0;
        }
        return new File(fd, name);
    }

    IContext* createSubcontext(const char* name)
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

    IInterface* lookup(const char* name)
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
            return static_cast<IFile*>(new Dir(newfd, name));
        }
        else
        {
            return static_cast<IFile*>(new File(newfd, name));
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

    IIterator* list(const char* name)
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

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IFile)
        {
            *objectPtr = static_cast<IFile*>(this);
        }
        else if (riid == IID_IBinding)
        {
            *objectPtr = static_cast<IBinding*>(this);
        }
        else if (riid == IID_IContext)
        {
            *objectPtr = static_cast<IContext*>(this);
        }
        else if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IContext*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
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

IInterface* Iterator::next()
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
            return static_cast<IFile*>(new Dir(newfd, ent->d_name));
        }
        else
        {
            return static_cast<IFile*>(new File(newfd, ent->d_name));
        }
    }
    offset = -1;
    return 0;
}

class Process : public IProcess
{
    Ref ref;
    int pid;

public:
    Process() : pid(0)
    {
    }

    // IProcess
    void kill() {}
    void start()
    {
    }
    void start(IFile* file)
    {
    }
    void start(IFile* file, const char* arguments)
    {
        if (int id = fork())
        {
            // parent
            if (0 < id)
            {
                pid = id;
            }
        }
        else
        {
            // child
            char* command = strdup(arguments);
            char* argv[32];
            int argc = 0;
            while (argc < 32)
            {
                argv[argc++] = command;
                char c;
                while (c = *command)
                {
                    if (c == '\\')
                    {
                        if (command[1] == ' ')
                        {
                            strcpy(command, command + 1);
                        }
                        ++command;
                    }
                    else if (isspace(c))
                    {
                        *command = '\0';
                        do
                        {
                            c = *++command;
                        } while (isspace(c));
                        break;
                    }
                    else
                    {
                        ++command;
                    }
                }
                if (c == '\0')
                {
                    break;
                }
            }
            argv[argc] = 0;

            int fd = ((File*) file)->getfd();
            //int fd = open("hello.elf", O_RDONLY);
            lseek(fd, 0, SEEK_SET);
            id = fexecve(fd, argv, environ);
            esReport("fexecve %d %d %d\n", id, errno, fd);
        }
    }
    int wait()
    {
        int result = 0;
        waitpid(pid, &result, 0);
        return result;
    }
    int getExitValue() { return 0; }
    bool hasExited() { return true; }
    void setRoot(IContext* root) {}
    void setIn(IStream* in) {}
    void setOut(IStream* out) {}
    void setError(IStream* error) {}

    // IInterface
    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IProcess)
        {
            *objectPtr = static_cast<IProcess*>(this);
        }
        else if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IProcess*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
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

class System : public ICurrentProcess
{
    Ref         ref;
    Stream*     in;
    Stream*     out;
    Stream*     error;
    IContext*   root;

public:
    System()
    {
        in = new Stream(stdin);
        out = new Stream(stdout);
        error = new Stream(stderr);

        IInterface* unknown = 0;
        esInit(&unknown);
        unknown->queryInterface(IID_IContext, (void**) &root);

        int fd = open(".", O_RDONLY);
        Dir* file = new Dir(fd, "");
        root->bind("file", static_cast<IContext*>(file));
        file->release();

        // Register CLSID_Process
        Handle<IClassStore> classStore(root->lookup("class"));
        IClassFactory* processFactory = new(ClassFactory<Process>);
        classStore->add(CLSID_Process, processFactory);
    }

    ~System()
    {
        root->release();
    }

    void exit(int status)
    {
        ::exit(status);
    }

    void* map(const void* start, long long length, unsigned int prot, unsigned int flags, IPageable* pageable, long long offset)
    {
    }

    void unmap(const void* start, long long length)
    {
    }

    ICurrentThread* currentThread()
    {
    }

    IThread* createThread(void* (*start)(void* param), void* param)
    {
    }

    void yield()
    {
    }

    IMonitor* createMonitor()
    {
    }

    IContext* getRoot()
    {
        root->addRef();
        return root;
    }

    IStream* getIn()
    {
        in->addRef();
        return in;
    }

    IStream* getOut()
    {
        out->addRef();
        return out;
    }

    IStream* getError()
    {
        error->addRef();
        return error;
    }

    void* setBreak(long long increment)
    {
    }

    long long getNow()
    {
        struct timespec ts;

        clock_gettime(CLOCK_REALTIME, &ts);
        DateTime now = DateTime(1970, 1, 1) + TimeSpan(ts.tv_sec * 10000000LL + ts.tv_nsec / 100);
        return now.getTicks();
    }

    bool trace(bool on)
    {
    }

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_ICurrentProcess)
        {
            *objectPtr = static_cast<ICurrentProcess*>(this);
        }
        else if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<ICurrentProcess*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
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

System current;

}

ICurrentProcess* System()
{
    return &current;
}
