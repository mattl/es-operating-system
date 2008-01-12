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
#include <termios.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/ioctl.h>
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

using namespace es;

int esInit(IInterface** nameSpace);

namespace
{

class Stream : public IStream
{
    Ref ref;
    int fd;

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
    // IInterface
    //

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == IStream::iid())
        {
            objectPtr = static_cast<IStream*>(this);
        }
        else if (riid == IInterface::iid())
        {
            objectPtr = static_cast<IStream*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
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

    IStream* getStream()
    {
        return new Stream(dup(fd));
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

    void setObject(IInterface* object)
    {
        esThrow(EACCES); // [check] appropriate?
    }

    int getName(char* name, int len)
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

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == IFile::iid())
        {
            objectPtr = static_cast<IFile*>(this);
        }
        else if (riid == IBinding::iid())
        {
            objectPtr = static_cast<IBinding*>(this);
        }
        else if (riid == IInterface::iid())
        {
            objectPtr = static_cast<IFile*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
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

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == IIterator::iid())
        {
            objectPtr = static_cast<IIterator*>(this);
        }
        else if (riid == IInterface::iid())
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
        int fd = open(name, O_CREAT | O_RDWR | O_TRUNC, 0777);
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

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == IFile::iid())
        {
            objectPtr = static_cast<IFile*>(this);
        }
        else if (riid == IBinding::iid())
        {
            objectPtr = static_cast<IBinding*>(this);
        }
        else if (riid == IContext::iid())
        {
            objectPtr = static_cast<IContext*>(this);
        }
        else if (riid == IInterface::iid())
        {
            objectPtr = static_cast<IContext*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
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
    void setInput(IStream* in) {}
    void setOutput(IStream* out) {}
    void setError(IStream* error) {}
    void setCurrent(IContext* context) {}

    IContext* getRoot()
    {
        esThrow(ENOSYS); // [check]
    }
    IStream* getInput()
    {
        esThrow(ENOSYS); // [check]
    }
    IStream* getOutput()
    {
        esThrow(ENOSYS); // [check]
    }
    IStream* getError()
    {
        esThrow(ENOSYS); // [check]
    }
    IContext* getCurrent()
    {
        esThrow(ENOSYS); // [check]
    }

    // IInterface
    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == IProcess::iid())
        {
            objectPtr = static_cast<IProcess*>(this);
        }
        else if (riid == IInterface::iid())
        {
            objectPtr = static_cast<IProcess*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
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

class System : public ICurrentProcess
{
    Ref         ref;
    Stream*     in;     // 0
    Stream*     out;    // 1
    Stream*     error;  // 2
    IContext*   root;
    IContext*   current;

public:
    System() :
        current(0)
    {
        struct termio tty;
        ioctl(0, TCGETA, &tty);
        tty.c_lflag &= ~(ICANON|ECHO);
        ioctl(0, TCSETAF, &tty);

        in = new Stream(0);
        out = new Stream(1);
        error = new Stream(2);

        IInterface* unknown = 0;
        esInit(&unknown);
        root = reinterpret_cast<IContext*>(unknown->queryInterface(IContext::iid()));

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

        struct termio tty;
        ioctl(0, TCGETA, &tty);
        tty.c_lflag |= ICANON|ECHO;
        ioctl(0, TCSETAF, &tty);
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

    IThread* createThread(const void* start, const void* param) // [check] start must be a function pointer.
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

    IStream* getInput()
    {
        in->addRef();
        return in;
    }

    IStream* getOutput()
    {
        out->addRef();
        return out;
    }

    IStream* getError()
    {
        error->addRef();
        return error;
    }

    void setCurrent(IContext* context)
    {
        if (context)
        {
            context->addRef();
        }
        if (current)
        {
            current->release();
        }
        current = context;
    }

    IContext* getCurrent()
    {
        if (current)
        {
            current->addRef();
        }
        return current;
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

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == ICurrentProcess::iid())
        {
            objectPtr = static_cast<ICurrentProcess*>(this);
        }
        else if (riid == IInterface::iid())
        {
            objectPtr = static_cast<ICurrentProcess*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
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

System current;

}

ICurrentProcess* System()
{
    return &current;
}
