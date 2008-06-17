/*
 * Copyright 2008 Google Inc.
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

#include <map>

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
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

#include <es/apply.h>
#include <es/broker.h>
#include <es/dateTime.h>
#include <es/handle.h>
#include <es/interlocked.h>
#include <es/objectTable.h>
#include <es/ref.h>
#include <es/reflect.h>
#include <es/timeSpan.h>
#include <es/classFactory.h>
#include <es/clsid.h>
#include <es/base/IProcess.h>

#include <sys/mman.h>

#include "core.h"
#include "posix_system.h"
#include "posix_video.h"

// #define VERBOSE

using namespace es;
using namespace posix;

extern Reflect::Interface& getInterface(const Guid& iid);

namespace
{

static const int MAX_EXPORT = 100;
static const int MAX_IMPORT = 100;

// for RPC
__thread int epfd = -1;
__thread int rpctag;
__thread std::map<pid_t, int>* socketMap;

class RpcStack
{
    static const int RPC_STACK_SIZE = 1024 * 1024;
    static const int ALIGN = 1 << 3;
    static __thread u8 rpcStackBase[RPC_STACK_SIZE];
    static __thread u8* rpcStack;

    u8* base;

public:
    RpcStack()
    {
        base = rpcStack;
    }

    ~RpcStack()
    {
        // Restore rpcStack
        rpcStack = base;
    }

    static void init()
    {
        rpcStack = rpcStackBase;
    }

    static void* alloc(size_t size)
    {
        size += ALIGN - 1;
        size &= ~(ALIGN - 1);
        if (size <= getFreeSize())
        {
            return rpcStack += size;
        }
        return 0;
    }

    static void* free(size_t size)
    {
        size += ALIGN - 1;
        size &= ~(ALIGN - 1);
        if (rpcStackBase <= (rpcStack - size))
        {
            return rpcStack -= size;
        }
        return rpcStackBase;
    }

    static void* top()
    {
        return rpcStack;
    }

    static size_t getFreeSize()
    {
        return rpcStackBase + RPC_STACK_SIZE - rpcStack;
    }
};

__thread u8 RpcStack::rpcStackBase[RPC_STACK_SIZE];
__thread u8* RpcStack::rpcStack;

struct ThreadCredential
{
    pid_t       pid;
    pthread_t   tid;
    u64         check;  // maybe optional
};

struct Capability
{
    pid_t   pid;
    int     object;
    u64     check;

    size_t hash() const
    {
        return hash(pid, object, check);
    }

    Capability& copy(const Capability& other)
    {
        pid = other.pid;
        object = other.object;
        check = other.check;
        return *this;
    }

    static size_t hash(pid_t pid, int object, u64 check)
    {
        return pid ^ object ^ check;
    }

    void report()
    {
        printf("Capability: <%d, %d, %llx>\n", pid, object, check);
    }
};

inline int operator==(const Capability& c1, const Capability& c2)
{
   return (c1.pid == c2.pid &&
           c1.object == c2.object &&
           c1.check == c2.check) ? true : false;
}

// System Commands (even: request, odd: reply)
static const int CMD_CHAN_REQ = 0;
static const int CMD_CHAN_RES = 1;
static const int CMD_FORK_REQ = 2;
static const int CMD_FORK_RES = 3;

struct CmdHdr
{
    int                 cmd;
    pid_t               pid;
};

struct CmdChanReq
{
    int                 cmd;     // CMD_CHAN_REQ
    pid_t               pid;
    int                 sockfd;
    Capability          cap;
    ThreadCredential    tc;
};

struct CmdChanRes
{
    int                 cmd;     // CMD_CHAN_RES
    pid_t               pid;
    ThreadCredential    tc;
};

struct CmdForkReq
{
    int                 cmd;     // CMD_FORK_REQ
    pid_t               pid;
};

struct CmdForkRes
{
    int                 cmd;     // CMD_FORK_RES
    pid_t               pid;
    Capability          in;
    Capability          out;
    Capability          error;
    Capability          root;
    Capability          current;

    void report()
    {
        printf("CmdForkRes:\n");
        in.report();
        out.report();
        error.report();
        root.report();
        current.report();
    }
};

union CmdUnion
{
    int         cmd;
    CmdHdr      hdr;
    CmdChanReq  chanReq;
    CmdChanRes  chanRes;
    CmdForkReq  forkReq;
    CmdForkRes  forkRes;
};

static const int RPC_REQ = 0;
static const int RPC_RES = 1;

// RPC request header

struct RpcHdr
{
    int         cmd;
    int         tag;
    pid_t       pid;
};

struct RpcReq
{
    int         cmd;
    int         tag;
    pid_t       pid;
    Capability  capability;
    unsigned    methodNumber;
    unsigned    paramCount;
    // Param    argv[];
    // Data

    Param* getArgv()
    {
        return reinterpret_cast<Param*>(this + 1);
    }

    void* getData()
    {
        return reinterpret_cast<void*>(getArgv() + paramCount);
    }
};

// RPC response header
struct RpcRes
{
    int         cmd;
    int         tag;
    pid_t       pid;
    unsigned    exceptionCode;
    Param       result;
    // Data

    void* getData()
    {
        return reinterpret_cast<void*>(this + 1);
    }
};

//
// Misc.
//

struct sockaddr* getSocketAddress(pid_t pid, struct sockaddr_un* sa)
{
    sa->sun_family = AF_UNIX;
    sprintf(sa->sun_path, "/tmp/es-socket-%u", pid);
    return reinterpret_cast<sockaddr*>(sa);
}

bool isMatch(pid_t pid, const struct sockaddr_un* sa)
{
    char sun_path[108]; // UNIX_PATH_MAX

    sprintf(sun_path, "/tmp/es-socket-%u", pid);
    return (strcmp(sun_path, sa->sun_path) == 0) ? true : false;
}

int* getRights(struct msghdr* msg, int* fdv, int maxfds)
{
    for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(msg);
         cmsg != 0;
         cmsg = CMSG_NXTHDR(msg, cmsg))
    {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
        {
            for (int i = 1; CMSG_LEN(i * sizeof(int)) <= cmsg->cmsg_len; ++i)
            {
                int fd = *((int*) CMSG_DATA(cmsg) + i - 1);
                if (i <= maxfds)
                {
                    *fdv++ = fd;
                }
                else
                {
                    close(fd);
                }
            }
        }
    }
    return fdv;
}

long long callRemote(const Capability& cap, unsigned methodNumber, va_list ap, Reflect::Method& method);
u64 getRandom();

typedef long long (*Method)(void* self, ...);

// XXX
f32 retF32(f32 v)
{
    return v;
}

// XXX
f64 retF64(f64 v)
{
    return v;
}

void printGuid(const Guid& guid)
{
    printf("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
           guid.Data1, guid.Data2, guid.Data3,
           guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
           guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

long long callRemote(void* self, void* base, int m, va_list ap);
Broker<callRemote, MAX_IMPORT> importer;

class Process;

class System : public ICurrentProcess
{
    int         sockfd; // usually 3. the control socket
    int         randfd;

    Ref         ref;
    IStream*    in;     // 0
    IStream*    out;    // 1
    IStream*    error;  // 2
    IContext*   root;
    IContext*   current;

    IThread*    front;

    std::map<pid_t, Process*>   children;   // list of child started processes. TODO must be guarded by a lock

    static void* focus(void* param);
    static void* servant(void* param);

    struct ExportKey
    {
        IInterface* object;
        Guid        iid;
    public:
        ExportKey(IInterface* object, const Guid& iid) :
            object(object),
            iid(iid)
        {
        }

        size_t hash() const
        {
            return hash(object, iid);
        }

        static size_t hash(IInterface* object, const Guid& iid)
        {
            return reinterpret_cast<const u32*>(&iid)[0] ^
                   reinterpret_cast<const u32*>(&iid)[1] ^
                   reinterpret_cast<const u32*>(&iid)[2] ^
                   reinterpret_cast<const u32*>(&iid)[3] ^
                   reinterpret_cast<size_t>(object);
        }
    };

    struct Exported
    {
        Ref         ref;
        IInterface* object;
        Guid        iid;
        u64         check;
        bool        doRelease;

    public:
        Exported(const ExportKey& key) :
            object(key.object),
            iid(key.iid),
            check(::getRandom())
        {
        }

        ~Exported()
        {
            // Do elease if necessary on objectTable
            if (doRelease)
            {
                object->release();
            }
        }

        bool isMatch(const ExportKey& key) const
        {
            return (object == key.object && iid == key.iid) ? true : false;
        }

        u64 getCheck() const
        {
            return check;
        }

        size_t hash() const
        {
            return reinterpret_cast<const u32*>(&iid)[0] ^
                   reinterpret_cast<const u32*>(&iid)[1] ^
                   reinterpret_cast<const u32*>(&iid)[2] ^
                   reinterpret_cast<const u32*>(&iid)[3] ^
                   reinterpret_cast<size_t>(object);
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
    };

    struct ImportKey
    {
        const Capability* capability;
        const Guid*       iid;
    public:
        ImportKey(const Capability* capability, const Guid& iid) :
            capability(capability),
            iid(&iid)
        {
        }

        size_t hash() const
        {
            return capability->hash();
        }
    };

    struct Imported
    {
        Ref         ref;
        Capability  capability;
        Guid        iid;
        bool        doRelease;

    public:
        Imported(const ImportKey& key) :
            ref(0),
            doRelease(false)
        {
            capability.copy(*key.capability);
            iid = *key.iid;
        }

        ~Imported()
        {
            if (doRelease)
            {
                // Make a release RPC request
            }
        }

        bool isMatch(const ImportKey& key) const
        {
            return (capability == *key.capability) ? true : false;
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
    };

    ObjectTable<ExportKey, Exported, MAX_EXPORT> exportedTable;
    ObjectTable<ImportKey, Imported, MAX_IMPORT> importedTable;

public:
    System() :
        current(0),
        exportedTable(createMonitor()),
        importedTable(createMonitor())
    {
        struct sockaddr_un sa;

        sockfd = socket(PF_UNIX, SOCK_DGRAM, 0);
        if (sockfd == -1)
        {
            perror("socket");
            exit(EXIT_FAILURE);
        }
        if (bind(sockfd, getSocketAddress(getpid(), &sa), sizeof sa) == -1)
        {
            perror("bind");
            exit(EXIT_FAILURE);
        }

        randfd = open("/dev/urandom", O_RDONLY);

        CmdForkReq cmd = {
            CMD_FORK_REQ,
            getpid()
        };
        ssize_t rc = sendto(sockfd, &cmd, sizeof cmd, MSG_DONTWAIT, getSocketAddress(getppid(), &sa), sizeof sa);
        if (rc != sizeof cmd)
        {
            // Root
            printf("Root %u\n", getpid());
            IInterface* unknown = 0;
            esInit(&unknown);
            root = reinterpret_cast<IContext*>(unknown->queryInterface(IContext::iid()));

            // Register CLSID_Process
            Handle<IClassStore> classStore(root->lookup("class"));
            IClassFactory* processFactory = new(ClassFactory<Process>);
            classStore->add(CLSID_Process, processFactory);

            // Register the pseudo framebuffer device
            try
            {
                Handle<IContext> device = root->lookup("device");
                VideoBuffer* buffer = new VideoBuffer(device);
            }
            catch (...)
            {
            }

            setCurrent(root);

            in = new Stream(0);
            out = new Stream(1);
            error = new Stream(2);

            int fd = open(".", O_RDONLY);
            Dir* file = new Dir(fd, "");
            root->bind("file", static_cast<IContext*>(file));
            file->release();

#ifdef __linux__
            struct termio tty;
            ioctl(0, TCGETA, &tty);
            tty.c_lflag &= ~(ICANON|ECHO);
            ioctl(0, TCSETAF, &tty);
#endif
        }
        else
        {
            // Non-root
            // XXX check sa
            printf("Non-root %d\n", getpid());
            esInitThread();

            for (int timeout = 3; 0 < timeout; --timeout)
            {
                CmdUnion cmd;
                struct sockaddr_un sa;

                ssize_t rc = receiveCommand(&cmd, &sa);
                if (rc == -1 || cmd.cmd != CMD_FORK_RES)
                {
                    // TODO maybe resend CMD_FORK_REQ?
                    continue;
                }

                // Import in, out, error, root, current
                cmd.forkRes.report();
                in = static_cast<IStream*>(importObject(cmd.forkRes.in, IStream::iid(), false));
                out = static_cast<IStream*>(importObject(cmd.forkRes.out, IStream::iid(), false));
                error = static_cast<IStream*>(importObject(cmd.forkRes.error, IStream::iid(), false));
                root = static_cast<IContext*>(importObject(cmd.forkRes.root, IContext::iid(), false));
                current = static_cast<IContext*>(importObject(cmd.forkRes.current, IContext::iid(), false));

                break;
            }

        }

        front = createThread((void*) focus, 0);
        front->start();
    }

    ~System()
    {
        if (in)
        {
            in->release();
        }
        if (out)
        {
            out->release();
        }
        if (error)
        {
            error->release();
        }
        if (current)
        {
            current->release();
        }
        if (root)
        {
            root->release();
        }

        if (0 <= sockfd)
        {
            close(sockfd);
        }
        char sun_path[108]; // UNIX_PATH_MAX
        sprintf(sun_path, "/tmp/es-socket-%u", getpid());
        printf("unlink %s\n", sun_path);
        unlink(sun_path);

#ifdef __linux__
        struct termio tty;
        ioctl(0, TCGETA, &tty);
        tty.c_lflag |= ICANON|ECHO;
        ioctl(0, TCSETAF, &tty);
#endif

        if (0 <= randfd)
        {
            close(randfd);
        }
    }

    void exit(int status)
    {
        ::exit(status);
    }

    void* map(const void* start, long long length, unsigned int prot, unsigned int flags, IPageable* pageable, long long offset)
    {
        if (Stream* stream = dynamic_cast<Stream*>(pageable))
        {
            return mmap(const_cast<void*>(start), length, prot, flags, stream->getfd(), offset);
        }
        else
        {
            return 0;
        }
    }

    void unmap(const void* start, long long length)
    {
        munmap(const_cast<void*>(start), length);
    }

    ICurrentThread* currentThread()
    {
    }

    // IThread* esCreateThread(void* (*start)(void* param), void* param)
    IThread* createThread(const void* start, const void* param) // [check] start must be a function pointer.
    {
        return esCreateThread((void* (*)(void*)) start, (void*) param);
    }

    void yield()
    {
    }

    IMonitor* createMonitor()
    {
        return esCreateMonitor();
    }

    IContext* getRoot()
    {
        if (root)
        {
            root->addRef();
        }
        return root;
    }

    IStream* getInput()
    {
        if (in)
        {
            in->addRef();
        }
        return in;
    }

    IStream* getOutput()
    {
        if (out)
        {
            out->addRef();
        }
        return out;
    }

    IStream* getError()
    {
        if (error)
        {
            error->addRef();
        }
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
        return sbrk((intptr_t) increment);
    }

    long long getNow()
    {
        struct timespec ts;
#ifdef __APPLE__
        struct timeval tv;

        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000;
#else
        clock_gettime(CLOCK_REALTIME, &ts);
#endif
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

    // misc.

    u64 getRandom() const
    {
        u64 v;

        if (0 <= randfd)
        {
            do {
                read(randfd, &v, sizeof v);
            } while (v == 0);
        }
        else
        {
            v = 0;
        }
        return v;
    }

    int exportObject(IInterface* object, const Guid& iid, Capability* cap, bool param)
    {
        // posix implementation directory transfers Dir, File, and Stream file descriptors
        if (Dir* dir = dynamic_cast<Dir*>(object))
        {
            if (iid == IContext::iid() || iid == IFile::iid() || iid == IBinding::iid())
            {
                cap->pid = getpid();
                cap->object = dup(dir->getfd());
                cap->check = 0; // i.e., local
                return cap->object;
            }
        }
        else if (File* file = dynamic_cast<File*>(object))
        {
            if (iid == IFile::iid() || iid == IBinding::iid())
            {
                cap->pid = getpid();
                cap->object = dup(file->getfd());
                cap->check = 0; // i.e., local
                return cap->object;
            }
        }
        else if (Stream* stream = dynamic_cast<Stream*>(object))
        {
            if (iid == IStream::iid() || iid == IPageable::iid())
            {
                cap->pid = getpid();
                cap->object = dup(stream->getfd());
                cap->check = 0; // i.e., local
                return cap->object;
            }
        }
        else if (Iterator* iterator= dynamic_cast<Iterator*>(object))
        {
            if (iid == IIterator::iid())
            {
                cap->pid = getpid();
                cap->object = dup(iterator->getfd());
                cap->check = 0; // i.e., local
                return cap->object;
            }
        }

        // If object has been exported, reuse the Exported entry by incrementing ref
        ExportKey key(object, iid);
        int i = exportedTable.add(key);
        if (0 <= i)
        {
            object->addRef();

            cap->pid = getpid();
            cap->object = i;
            Exported* exported = exportedTable.get(i);
            cap->check = exported->getCheck();
            exportedTable.put(i);
        }
        return i;
    }

    IInterface* importObject(const Capability& cap, const Guid& iid, bool param)
    {
#ifdef VERBOSE
        printf("importObject: ");
        printGuid(iid);
        printf(" : %d\n", param);
#endif

        if (cap.object < 0)
        {
            return 0;
        }

        if (cap.check == 0)
        {
            if (iid == IContext::iid())
            {
                return static_cast<IContext*>(new Dir(cap.object, ""));
            }
            if (iid == IFile::iid())
            {
                return static_cast<IFile*>(new File(cap.object, ""));
            }
            if (iid == IStream::iid())
            {
                return static_cast<IStream*>(new Stream(cap.object));
            }
            if (iid == IIterator::iid())   // TODO bk
            {
                return static_cast<IIterator*>(new Iterator(cap.object));
            }
            return 0;
        }

        ImportKey key(&cap, iid);
        int i = importedTable.add(key);
        if (0 <= i)
        {
            if (!param)
            {
                Imported* imported = importedTable.get(i);
                ASSERT(imported);
                imported->addRef();
                importedTable.put(i);
            }
            return reinterpret_cast<IInterface*>(&(importer.getInterfaceTable())[i]);
        }
        else
        {
            if (!param)
            {
                // TODO call release
                // callRemote(cap, method, ap);
            }
            return 0;
        }
    }

    void addChild(pid_t pid, Process* child);
    Process* getChild(pid_t pid);
    void removeChild(pid_t pid);

    ssize_t receiveCommand(CmdUnion* cmd, struct sockaddr_un* sa, int flags = 0)
    {
        struct msghdr       msg;
        struct iovec        iov;
        struct cmsghdr*     cmsg;
        int                 maxfds = 0; // max. # of file descriptors to receive
        int*                fds;
        unsigned char       buf[CMSG_SPACE(8 * sizeof(int))];

        msg.msg_name = sa;
        msg.msg_namelen = sizeof(struct sockaddr_un);
        iov.iov_base = cmd;
        iov.iov_len = sizeof(CmdUnion);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = buf;
        msg.msg_controllen = CMSG_LEN(8 * sizeof(int));
        msg.msg_flags = 0;

        ssize_t rc = recvmsg(sockfd, &msg, flags);
        if (rc == -1)
        {
            return -1;
        }

        if (rc < sizeof(CmdHdr) || !isMatch(cmd->hdr.pid, sa))
        {
            errno = EBADMSG;
            rc = -1;
        }
        else
        {
            switch (cmd->cmd)
            {
                case CMD_CHAN_REQ:
                    if (sizeof(CmdChanReq) != rc)
                    {
                        errno = EBADMSG;
                        rc = -1;
                        break;
                    }
                    fds = &cmd->chanReq.sockfd;
                    maxfds = 1;
                    break;
                case CMD_CHAN_RES:
                    if (sizeof(CmdChanRes) != rc)
                    {
                        errno = EBADMSG;
                        rc = -1;
                        break;
                    }
                    break;
                case CMD_FORK_REQ:
                    if (sizeof(CmdForkReq) != rc)
                    {
                        errno = EBADMSG;
                        rc = -1;
                        break;
                    }
                    break;
                case CMD_FORK_RES:
                    if (sizeof(CmdForkRes) != rc)
                    {
                        errno = EBADMSG;
                        rc = -1;
                        break;
                    }
                    break;
            }
        }

        getRights(&msg, fds, maxfds);

        return rc;
    }

    // Look up the exportedTable
    Exported* getExported(Capability& cap)
    {
        if (cap.pid != getpid())
        {
            return 0;
        }
        Exported* exported = exportedTable.get(cap.object);
        if (!exported)
        {
            return 0;
        }
        if (exported->check != cap.check)
        {
            exportedTable.put(cap.object);
            return 0;
        }
        return exported;
    }

    // RpcRes res = ;
    int callLocal(RpcReq* hdr, int* fdv, int s)
    {
        RpcRes res = { RPC_RES, hdr->tag, getpid(), 0 };
        size_t resultSize = 0;

        Exported* exported = getExported(hdr->capability);
        if (!exported)
        {
            res.exceptionCode = EINVAL;
            return 0;   // TODO goto
        }

        unsigned methodNumber = hdr->methodNumber;

        // Determine the type of interface and which method is being invoked.
        Reflect::Interface interface = getInterface(exported->iid);

        // If this interface inherits another interface,
        // methodNumber is checked accordingly.
        if (interface.getInheritedMethodCount() + interface.getMethodCount() <= methodNumber)
        {
            res.exceptionCode = ENOSYS;
            return 0;   // TODO goto
        }
        unsigned baseMethodCount;
        Reflect::Interface super(interface);
        for (;;)
        {
            baseMethodCount = super.getInheritedMethodCount();
            if (baseMethodCount <= methodNumber)
            {
                break;
            }
            super = getInterface(super.getSuperIid());
        }
        Reflect::Method method(Reflect::Method(super.getMethod(methodNumber - baseMethodCount)));

        // TODO Review later. Probably wrong...
        if (super.getIid() == IInterface::iid())
        {
            int count;

            switch (methodNumber - baseMethodCount)
            {
            case 1: // addRef
                count = exported->addRef();
                if (1 != count)
                {
                    res.result.s32 = count;
                    res.result.cls = Param::S32;
                    return 0;
                }
                exportedTable.get(hdr->capability.object);
                break;
            case 2: // release
                count = exported->release();
                exportedTable.put(hdr->capability.object);
                if (0 <= count)
                {
                    exportedTable.put(hdr->capability.object);
                }
                res.result.s32 = count;
                res.result.cls = Param::S32;
                break;
            }
        }

        Guid iid = IInterface::iid();
        Method** object = reinterpret_cast<Method**>(exported->object);
        int argc = hdr->paramCount;
        Param* argv = hdr->getArgv();
        Param* argp = argv;

        // Set this
        argp->ptr = exported->object;
        ++argp;

        // Reserve space from rpcStack to store result
        void* resultPtr = 0;
        Reflect::Type returnType = method.getReturnType();
        switch (returnType.getType())
        {
        case Ent::TypeSequence:
            // int op(xxx* buf, int len, ...);
            argp->ptr = resultPtr = RpcStack::alloc(returnType.getSize() * argp[1].s32);
            argp->cls = Param::PTR;
            ++argp;
            ++argp;
            break;
        case Ent::SpecString:
            // int op(char* buf, int len, ...);
            argp->ptr = resultPtr = RpcStack::alloc(argp[1].s32);
            argp->cls = Param::PTR;
            ++argp;
            ++argp;
            break;
        case Ent::SpecWString:
            // int op(wchar_t* buf, int len, ...);
            argp->ptr = resultPtr = RpcStack::alloc(sizeof(wchar_t) * argp[1].s32);
            argp->cls = Param::PTR;
            ++argp;
            ++argp;
            break;
        case Ent::SpecUuid:
        case Ent::TypeStructure:
            // void op(struct* buf, ...);
            resultSize = returnType.getSize();
            argp->ptr = resultPtr = RpcStack::alloc(resultSize);
            argp->cls = Param::PTR;
            ++argp;
            break;
        case Ent::TypeArray:
            // void op(xxx[x] buf, ...);
            resultSize = returnType.getSize();
            argp->ptr = resultPtr = RpcStack::alloc(resultSize);
            argp->cls = Param::PTR;
            ++argp;
            break;
        case Ent::TypeInterface:
        case Ent::SpecObject:
            resultSize = sizeof(Capability);
            resultPtr = RpcStack::alloc(resultSize);
            break;
        }

        // Convert argp->size to argp->ptr
        u8* data = static_cast<u8*>(hdr->getData());    // TODO review alignment issues
        size_t size;
        for (int i = 0; i < method.getParameterCount(); ++i, ++argp)
        {
            Reflect::Parameter param(method.getParameter(i));
            Reflect::Type type(param.getType());
            assert(param.isInput());

            switch (type.getType())
            {
            case Ent::TypeSequence:
                // xxx* buf, int len, ...
                size = returnType.getSize() * argp->size;
                argp->ptr = data;
                argp->cls = Param::PTR;
                data += size;
                ++argp;
                break;
            case Ent::SpecString:
                size = argp->size;
                argp->ptr = data;
                argp->cls = Param::PTR;
                data += size;
                break;
            case Ent::SpecWString:
                size = sizeof(wchar_t) * argp->size;
                argp->ptr = data;
                argp->cls = Param::PTR;
                data += size;
                break;
            case Ent::SpecUuid:
                iid = argp->guid;
                break;
            case Ent::TypeStructure:
            case Ent::TypeArray:
                size = argp->size;
                argp->ptr = data;
                argp->cls = Param::PTR;
                data += size;
                break;
            case Ent::TypeInterface:
                iid = type.getInterface().getIid();
                // FALL THROUGH
            case Ent::SpecObject:
                if (argp->ptr)
                {
                    // Import object
                    Capability* cap = reinterpret_cast<Capability*>(data);
                    if (cap->check == 0)
                    {
                        cap->object = *fdv++;   // TODO check range
                    }
                    IInterface* object = importObject(*cap, iid, true);
                    data += sizeof(Capability);
                }
                break;
            case Ent::SpecBool:
            case Ent::SpecAny:
            case Ent::SpecS8:
            case Ent::SpecS16:
            case Ent::SpecS32:
            case Ent::SpecU8:
            case Ent::SpecU16:
            case Ent::SpecU32:
            case Ent::SpecS64:
            case Ent::SpecU64:
            case Ent::SpecF32:
            case Ent::SpecF64:
                break;
            default:
                break;
            }
        }

        // TODO Close unused fdv

        int fd;
        fdv = &fd;
        int* fdp = fdv;

        // TODO catch exception while applying
        switch (returnType.getType())
        {
        case Ent::SpecAny:  // XXX x86 specific
        case Ent::SpecBool:
        case Ent::SpecChar:
        case Ent::SpecWChar:
        case Ent::SpecS8:
        case Ent::SpecS16:
        case Ent::SpecS32:
        case Ent::SpecU8:
        case Ent::SpecU16:
        case Ent::SpecU32:
            res.result.s32 = applyS32(argc, argv, (s32 (*)()) ((*object)[methodNumber]));
            res.result.cls = Param::S32;
            break;
        case Ent::SpecS64:
        case Ent::SpecU64:
            res.result.s64 = applyS64(argc, argv, (s64 (*)()) ((*object)[methodNumber]));
            res.result.cls = Param::S64;
            break;
        case Ent::SpecF32:
            res.result.f32 = applyF32(argc, argv, (f32 (*)()) ((*object)[methodNumber]));
            res.result.cls = Param::F32;
            break;
        case Ent::SpecF64:
            res.result.f64 = applyF64(argc, argv, (f64 (*)()) ((*object)[methodNumber]));
            res.result.cls = Param::F64;
            break;
        case Ent::SpecString:
            res.result.s32 = applyS32(argc, argv, (s32 (*)()) ((*object)[methodNumber]));
            res.result.cls = Param::S32;
            resultSize = 1 + res.result.s32;
            break;
        case Ent::SpecWString:
            res.result.s32 = applyS32(argc, argv, (s32 (*)()) ((*object)[methodNumber]));
            res.result.cls = Param::S32;
            resultSize = sizeof(wchar_t) * (1 + res.result.s32);
            break;
        case Ent::TypeSequence:
            res.result.s32 = applyS32(argc, argv, (s32 (*)()) ((*object)[methodNumber]));
            res.result.cls = Param::S32;
            resultSize = sizeof(returnType.getSize() * res.result.s32);
            break;
        case Ent::TypeInterface:
            iid = returnType.getInterface().getIid();
            // FALL THROUGH
        case Ent::SpecObject:
            res.result.ptr = applyPTR(argc, argv, (const void* (*)()) ((*object)[methodNumber]));
            res.result.cls = Param::PTR;
            if (res.result.ptr)
            {
                Capability* cap = static_cast<Capability*>(resultPtr);
                exportObject((IInterface*) res.result.ptr, iid, cap, false);   // TODO error check
                if (cap->check == 0)
                {
                    *fdp++ = cap->object;
                }
#ifdef VERBOSE
                printf("<< ");
                cap->report();
#endif
            }
            break;
        case Ent::TypeArray:
        case Ent::SpecVoid:
            applyS32(argc, argv, (s32 (*)()) ((*object)[methodNumber]));
            res.result.s32 = 0;
            res.result.cls = Param::S32;
            break;
        }

        // Send response
        struct iovec iov[2];
        iov[0].iov_base = &res;
        iov[0].iov_len = sizeof(RpcRes);

        // Invoke method
        struct msghdr msg;

        msg.msg_name = 0;
        msg.msg_namelen = 0;

        msg.msg_iov = iov;
        if (resultSize == 0)
        {
            msg.msg_iovlen = 1;
        }
        else
        {
            iov[1].iov_base = resultPtr;
            iov[1].iov_len = resultSize;
            msg.msg_iovlen = 2;
        }

        unsigned char buf[CMSG_SPACE(sizeof(int))];
        if (fdv < fdp)
        {
            int size = sizeof(int) * (fdp - fdv);
            msg.msg_control = buf;
            msg.msg_controllen = CMSG_SPACE(size);

            struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
            cmsg->cmsg_level = SOL_SOCKET;
            cmsg->cmsg_type = SCM_RIGHTS;
            cmsg->cmsg_len = CMSG_LEN(size);
            memcpy((int*) CMSG_DATA(cmsg), fdv, size);
            msg.msg_controllen = cmsg->cmsg_len;
        }
        else
        {
            msg.msg_control = 0;
            msg.msg_controllen = 0;
        }

        msg.msg_flags = 0;

#ifdef VERBOSE
        printf("Send RpcRes: %p %zd\n", resultPtr, resultSize);
        esDump(&res, sizeof(RpcRes));
#endif

        int rc = sendmsg(s, &msg, 0);

        exportedTable.put(hdr->capability.object);
        return rc;
    }

    long long callRemote(int interfaceNumber, int methodNumber, va_list ap)
    {
        long long result = 0;
        int error = 0;

        Imported* imported = importedTable.get(interfaceNumber);
        if (!imported)
        {
            throw SystemException<EBADF>();
        }

        // Determine the type of interface and which method is being invoked.
        Reflect::Interface interface = getInterface(imported->iid);

        // If this interface inherits another interface,
        // methodNumber is checked accordingly.
        if (interface.getInheritedMethodCount() + interface.getMethodCount() <= methodNumber)
        {
            throw SystemException<ENOSYS>();
        }
        unsigned baseMethodCount;
        Reflect::Interface super(interface);
        for (;;)
        {
            baseMethodCount = super.getInheritedMethodCount();
            if (baseMethodCount <= methodNumber)
            {
                break;
            }
            super = getInterface(super.getSuperIid());
        }
        Reflect::Method method(Reflect::Method(super.getMethod(methodNumber - baseMethodCount)));

        if (super.getIid() == IInterface::iid())
        {
            int count;

            switch (methodNumber - baseMethodCount)
            {
            case 1: // addRef
                count = imported->addRef();
                if (1 != count)
                {
                    return count;
                }
                importedTable.get(interfaceNumber);
                break;
            case 2: // release
                count = imported->release();
                importedTable.put(interfaceNumber);
                if (0 <= count)
                {
                    importedTable.put(interfaceNumber);
                }
                return count;
                break;
            }
        }

        result = ::callRemote(imported->capability, methodNumber, ap, method);

        importedTable.put(interfaceNumber);

        if (error)
        {
            esThrow(error);
        }
        return result;
    }

    int makeConnection(const Capability& cap)
    {
        struct sockaddr_un  sa;
        struct msghdr       msg;
        struct iovec        iov;
        struct cmsghdr*     cmsg;
        int                 pair[2];

        if (socketpair(PF_UNIX, SOCK_DGRAM, 0, pair) == -1)
        {
            perror("socketpair");
            esThrow(errno);
        }

        msg.msg_name = getSocketAddress(cap.pid, &sa);
        msg.msg_namelen = sizeof sa;

        CmdChanReq cmd =
        {
            CMD_CHAN_REQ,
            getpid(),
            pair[1]
            // TODO - set ThreadCredential here
        };
        iov.iov_base = &cmd;
        iov.iov_len = sizeof cmd;

        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        unsigned char buf[CMSG_SPACE(sizeof(int))];
        msg.msg_control = buf;
        msg.msg_controllen = sizeof buf;
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        *(int*) CMSG_DATA(cmsg) = pair[1];
        msg.msg_controllen = cmsg->cmsg_len;

        msg.msg_flags = 0;

        ssize_t rc = sendmsg(sockfd, &msg, 0);
        if (rc == -1)
        {
            close(pair[0]);
            close(pair[1]);
            esThrow(errno);
        }
        // perhaps wait here by recvmsg(pair[1], ...)  // Note waiting not on sockfd

        // add pair[0] to the poll list
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLOUT;
        event.data.fd = pair[0];
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, pair[0], &event) == -1)
        {
            perror("epoll_ctl");
            exit(EXIT_FAILURE);
        }
        (*socketMap)[cap.pid] = pair[0];

        return pair[0];
    }

};

System current;

class Process : public IProcess
{
    Ref         ref;
    IStream*    in;     // 0
    IStream*    out;    // 1
    IStream*    error;  // 2
    IContext*   root;
    IContext*   current;
    int         pid;

    bool        exited;
    int         exitValue;

    CmdForkRes  cmd;

public:
    Process() :
        pid(-1),
        in(::current.getInput()),
        out(::current.getOutput()),
        error(::current.getError()),
        root(::current.getRoot()),
        current(::current.getCurrent()),
        exitValue(0),
        exited(false)
    {
    }

    ~Process()
    {
        if (in)
        {
            in->release();
        }
        if (out)
        {
            out->release();
        }
        if (error)
        {
            error->release();
        }
        if (current)
        {
            current->release();
        }
        if (root)
        {
            root->release();
        }
    }

    // IProcess
    void kill()
    {
    }

    void start()
    {
    }

    void start(IFile* file)
    {
        start(file, "");
    }

    void start(IFile* file, const char* arguments)
    {
        // export in, out, error, root, context to child
        cmd.cmd = CMD_FORK_RES;
        cmd.pid = getpid();
        ::current.exportObject(in, IStream::iid(), &cmd.in, false);
        ::current.exportObject(out, IStream::iid(), &cmd.out, false);
        ::current.exportObject(error, IStream::iid(), &cmd.error, false);
        ::current.exportObject(current, IContext::iid(), &cmd.current, false);
        ::current.exportObject(root, IContext::iid(), &cmd.root, false);
        cmd.report();

        int pfd[2];
        pipe(pfd);
        char token;

        if (pid = fork())
        {
            // parent
            close(pfd[0]);

            if (0 <= pid)
            {
                ::current.addChild(pid, this);
            }

            // Synchronize with the child
            token = 0;
            write(pfd[1], &token, sizeof token);
            close(pfd[1]);

            if (cmd.in.check == 0)
            {
                close(cmd.in.object);
            }
            if (cmd.out.check == 0)
            {
                close(cmd.out.object);
            }
            if (cmd.error.check == 0)
            {
                close(cmd.error.object);
            }
            if (cmd.current.check == 0)
            {
                close(cmd.current.object);
            }
            if (cmd.root.check == 0)
            {
                close(cmd.root.object);
            }
        }
        else
        {
            // child
            close(pfd[1]);

            // Synchronize with the parent
            read(pfd[0], &token, sizeof token);
            close(pfd[0]);

            // Unrelated files should be closed by the settings of fcntl FD_CLOEXEC after fexecve
            close(3);

            printf("ppid: %d\n", getppid());

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
            lseek(fd, 0, SEEK_SET);
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 3)
            fexecve(fd, argv, environ);
#endif
            exit(EXIT_FAILURE);
        }
    }

    int wait()
    {
        if (!exited)
        {
            waitpid(pid, &exitValue, 0);
            exited = true;
        }
        return exitValue;
    }

    int getExitValue()
    {
        return exitValue;
    }

    bool hasExited()
    {
        return exited;
    }

    void setRoot(IContext* context)
    {
        if (context)
        {
            context->addRef();
        }
        if (root)
        {
            root->release();
        }
        root = context;
    }

    void setInput(IStream* stream)
    {
        if (stream)
        {
            stream->addRef();
        }
        if (in)
        {
            in->release();
        }
        in = stream;
    }

    void setOutput(IStream* stream)
    {
        if (stream)
        {
            stream->addRef();
        }
        if (out)
        {
            out->release();
        }
        out = stream;
    }

    void setError(IStream* stream)
    {
        if (stream)
        {
            stream->addRef();
        }
        if (error)
        {
            error->release();
        }
        error = stream;
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

    IContext* getRoot()
    {
        if (root)
        {
            root->addRef();
        }
        return root;
    }

    IStream* getInput()
    {
        if (in)
        {
            in->addRef();
        }
        return in;
    }

    IStream* getOutput()
    {
        if (out)
        {
            out->addRef();
        }
        return out;
    }

    IStream* getError()
    {
        if (error)
        {
            error->addRef();
        }
        return error;
    }

    IContext* getCurrent()
    {
        if (current)
        {
            current->addRef();
        }
        return current;
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

    // misc.

    const CmdForkRes& getCmdForkRes() const
    {
        return cmd;
    }
};

// Process system commands
void* System::focus(void* param)
{
    printf("front\n");
    for (;;)
    {
        CmdUnion cmd;
        struct sockaddr_un sa;

        ssize_t rc = ::current.receiveCommand(&cmd, &sa);
        if (rc == -1)
        {
            ::exit(EXIT_FAILURE);
            continue;
        }

        switch (cmd.cmd)
        {
        case CMD_CHAN_REQ: {
            int epfd = epoll_create(1);
            if (epfd < 0)
            {
                perror("epoll_create");
                ::exit(EXIT_FAILURE);
            }
            struct epoll_event event;
            event.events = EPOLLIN | EPOLLOUT;
            event.data.fd = cmd.chanReq.sockfd;
            if (epoll_ctl(epfd, EPOLL_CTL_ADD, cmd.chanReq.sockfd, &event) == -1)
            {
                perror("epoll_ctl");
                ::exit(EXIT_FAILURE);
            }
            // TODO check ThreadCredential and if the thread is created already, just ad fd to its socketMap.
            IThread* thread = ::current.createThread((void*) servant, (void*) epfd);
            // client is not waited right now - sendmsg(fd, CMD_CHAN_RES);
            thread->start();
            break;
        }
        case CMD_CHAN_RES:
            break;
        case CMD_FORK_REQ: {
            // Look up the corrensponding Process
            if (Process* child = ::current.getChild(cmd.forkReq.pid))
            {
                // Send CmdForkRes
                ssize_t rc = sendto(::current.sockfd, &child->getCmdForkRes(), sizeof(CmdForkRes),
                                    MSG_DONTWAIT, getSocketAddress(cmd.forkReq.pid, &sa), sizeof sa);
                if (rc == sizeof(CmdForkRes))
                {
                    ::current.removeChild(cmd.forkReq.pid);
                }
            }
            else
            {
                // TODO
                printf("Oops %s %d\n", __FILE__, __LINE__);
            }
            break;
        }
        case CMD_FORK_RES:
            break;
        }
    }
}

void System::addChild(pid_t pid, Process* child)
{
    // TODO Synchronized<es::IMonitor*> method(monitor);

    child->addRef();
    children.insert(std::pair<pid_t, Process*>(pid, child));
}

Process* System::getChild(pid_t pid)
{
    // TODO Synchronized<es::IMonitor*> method(monitor);

    std::map<pid_t, Process*>::iterator it = children.find(pid);
    if (it != children.end())
    {
        return (*it).second;
    }
    return 0;
}

void System::removeChild(pid_t pid)
{
    // TODO Synchronized<es::IMonitor*> method(monitor);

    std::map<pid_t, Process*>::iterator it = children.find(pid);
    if (it != children.end())
    {
        (*it).second->release();
        children.erase(it);
    }
}

u64 getRandom()
{
    return current.getRandom();
}

RpcHdr* receiveMessage(struct msghdr* msg, int* fdv, int*& fdmax, int* s)
{
    RpcHdr* hdr;
    unsigned char buf[CMSG_SPACE(8 * sizeof(int))];
    struct epoll_event event;

    // wait for the reply. note the thread might receive another request by recursive call, etc.
    for (;;)
    {
        int fdCount = epoll_wait(epfd, &event, 1, -1);
        if (fdCount == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        if (fdCount != 1)
        {
            continue;
        }

        struct iovec iov;
        msg->msg_name = 0;
        msg->msg_namelen = 0;
        iov.iov_base = RpcStack::top();
        iov.iov_len = RpcStack::getFreeSize();
        msg->msg_iov = &iov;
        msg->msg_iovlen = 1;
        msg->msg_control = buf;
        msg->msg_controllen = sizeof buf;
        msg->msg_flags = 0;
        int rc = recvmsg(event.data.fd, msg, 0);

        if (0 <= rc)
        {
#ifdef VERBOSE
            printf("Recv RpcMsg: %d\n", rc);
            esDump(RpcStack::top(), rc);
#endif
            fdmax = getRights(msg, fdv, 8);
        }
        else
        {
            fdmax = fdv;
        }

        if (sizeof(RpcHdr) <= rc)
        {
            // TODO check trunk etc.
            hdr = reinterpret_cast<RpcHdr*>(iov.iov_base);
            if (hdr->cmd == RPC_REQ && sizeof(RpcReq) <= rc)
            {
                RpcStack::alloc(rc);
                break;
            }
            else if (hdr->cmd == RPC_RES && sizeof(RpcReq) <= rc)
            {
                RpcStack::alloc(rc);
                break;
            }
        }

        // Close unused rights
        for (int* p = fdv; p < fdmax; ++p)
        {
            close(*p);
        }
    }

    std::map<pid_t, int>::iterator it = socketMap->find(hdr->pid);
    if (it == socketMap->end())
    {
        (*socketMap)[hdr->pid] = event.data.fd;
    }

    *s = event.data.fd;
    return hdr;
}

long long callRemote(void* self, void* base, int methodNumber, va_list ap)
{
    unsigned interfaceNumber = static_cast<void**>(self) - static_cast<void**>(base);

    return current.callRemote(interfaceNumber, methodNumber, ap);
}

long long callRemote(const Capability& cap, unsigned methodNumber, va_list ap, Reflect::Method& method)
{
    if (epfd < 0)
    {
        epfd = epoll_create(1);
        if (epfd < 0)
        {
            perror("epoll_create");
            exit(EXIT_FAILURE);
        }

        // TODO Register map<tid_t, int epfd> to system
        socketMap = new std::map<pid_t, int>;   // TODO clean up after thread termination
        RpcStack::init();
    }

    int s;  // socket to be used

    std::map<pid_t, int>::iterator it = socketMap->find(cap.pid);
    if (it != socketMap->end())
    {
        s = (*it).second;
    }
    else
    {
        s = current.makeConnection(cap);
    }

    // Pack arguments
    int tag = ++rpctag;
    struct
    {
        RpcReq  req;
        Param   argv[9];
    } rpcmsg = { RPC_REQ, tag, getpid(), cap, methodNumber };
    Param* argp = rpcmsg.argv;
    Capability caps[8];
    Capability* capp = caps;

    struct iovec iov[9];
    struct iovec* iop = iov + 1;
    int fdv[8];
    int* fdp = fdv;

    // In the following implementation, we assume no out or inout attribute is
    // used for parameters.
    // Set up parameters
    Guid iid = IInterface::iid();

    // Set this
    argp->ptr = 0;      // to be filled by the server
    argp->cls = Param::PTR;
    ++argp;

    Reflect::Type returnType = method.getReturnType();
    switch (returnType.getType())
    {
    case Ent::TypeSequence:
        // int op(xxx* buf, int len, ...);
        argp->ptr = va_arg(ap, void*);
        argp->cls = Param::PTR;
        ++argp;
        argp->s32 = va_arg(ap, int);
        argp->cls = Param::S32;
        ++argp;
        break;
    case Ent::SpecString:
    case Ent::SpecWString:
        // int op(xxx* buf, int len, ...);
        argp->ptr = va_arg(ap, void*);
        argp->cls = Param::PTR;
        ++argp;
        argp->s32 = va_arg(ap, int);
        argp->cls = Param::S32;
        ++argp;
        break;
    case Ent::SpecUuid:
    case Ent::TypeStructure:
        // void op(struct* buf, ...);
        argp->ptr = va_arg(ap, void*);
        argp->cls = Param::PTR;
        ++argp;
        break;
    case Ent::TypeArray:
        // void op(xxx[x] buf, ...);
        argp->ptr = va_arg(ap, void*);
        argp->cls = Param::PTR;
        ++argp;
        break;
    }

    for (int i = 0; i < method.getParameterCount(); ++i, ++argp)
    {
        Reflect::Parameter param(method.getParameter(i));
        Reflect::Type type(param.getType());
        assert(param.isInput());

        switch (type.getType())
        {
        case Ent::TypeSequence:
            // xxx* buf, int len, ...
            iop->iov_base = va_arg(ap, void*);
            argp->cls = Param::PTR;
            ++argp;
            argp->s32 = va_arg(ap, int);
            argp->cls = Param::S32;
            (argp - 1)->size = iop->iov_len = type.getSize() * argp->s32;
            ++iop;
            break;
        case Ent::SpecString:
            iop->iov_base = va_arg(ap, char*);
            if (iop->iov_base)
            {
                argp->size = iop->iov_len = strlen(static_cast<char*>(iop->iov_base)) + sizeof(char);
                ++iop;
            }
            else
            {
                argp->size = 0;
            }
            argp->cls = Param::PTR;
            break;
        case Ent::SpecWString:
            iop->iov_base = va_arg(ap, wchar_t*);
            if (iop->iov_base)
            {
                argp->size = iop->iov_len = sizeof(wchar_t) * (wcslen(static_cast<wchar_t*>(iop->iov_base)) + 1);
                ++iop;
            }
            else
            {
                argp->size = 0;
            }
            argp->cls = Param::PTR;
            break;
        case Ent::SpecUuid:
            memcpy(&argp->guid, va_arg(ap, Guid*), sizeof(Guid));
            argp->cls = Param::REF;
            iid = argp->guid;
            break;
        case Ent::TypeStructure:
            iop->iov_base = va_arg(ap, void*);
            argp->size = iop->iov_len = type.getSize();
            ++iop;
            argp->cls = Param::PTR;
            break;
        case Ent::TypeArray:
            iop->iov_base = va_arg(ap, void*);
            argp->size = iop->iov_len = type.getSize();
            ++iop;
            argp->cls = Param::PTR;
            break;
        case Ent::TypeInterface:
            iid = type.getInterface().getIid();
            // FALL THROUGH
        case Ent::SpecObject: {
            IInterface* object = va_arg(ap, IInterface*);
            current.exportObject(object, iid, capp, true);  // TODO check error
            iop->iov_base = capp;
            argp->size = iop->iov_len = sizeof(Capability);
            if (capp->check == 0)
            {
                *fdp++ = capp->object;
            }
            ++capp;
            ++iop;
            argp->cls = Param::PTR;
            break;
        }
        case Ent::SpecBool:
            argp->s32 = va_arg(ap, int);
            argp->cls = Param::S32;
            break;
        case Ent::SpecAny:
            argp->ptr = va_arg(ap, void*);
            argp->cls = Param::PTR;
            break;
        case Ent::SpecS8:
        case Ent::SpecS16:
        case Ent::SpecS32:
        case Ent::SpecU8:
        case Ent::SpecU16:
        case Ent::SpecU32:
            argp->s32 = va_arg(ap, u32);
            argp->cls = Param::S32;
            break;
        case Ent::SpecS64:
        case Ent::SpecU64:
            argp->s64 = va_arg(ap, u64);
            argp->cls = Param::S64;
            break;
        case Ent::SpecF32:
            argp->s32 = va_arg(ap, s32);    // XXX work on X86 only probably
            argp->cls = Param::F32;
            break;
        case Ent::SpecF64:
            argp->f64 = va_arg(ap, double);
            argp->cls = Param::F64;
            break;
        default:
            break;
        }
    }

    int argc = argp - rpcmsg.argv;
    rpcmsg.req.paramCount = argc;
    iov[0].iov_base = &rpcmsg;
    iov[0].iov_len = sizeof(RpcReq) + sizeof(Param) * argc;

#ifdef VERBOSE
    printf("Send RpcReq: %d %d\n", s, argc);
    esDump(&rpcmsg, sizeof(RpcReq) + sizeof(Param) * argc);
#endif

    // Invoke method
    struct msghdr msg;

    msg.msg_name = 0;
    msg.msg_namelen = 0;

    msg.msg_iov = iov;
    msg.msg_iovlen = iop - iov;

    unsigned char buf[CMSG_SPACE(sizeof fdv)];
    if (fdv < fdp)
    {
        int size = sizeof(int) * (fdp - fdv);
        msg.msg_control = buf;
        msg.msg_controllen = CMSG_SPACE(size);

        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(size);
        memcpy((int*) CMSG_DATA(cmsg), fdv, size);
        msg.msg_controllen = cmsg->cmsg_len;
    }
    else
    {
        msg.msg_control = 0;
        msg.msg_controllen = 0;
    }

    msg.msg_flags = 0;

    if (sendmsg(s, &msg, 0) == -1)
    {
        // TODO cancel export
        esThrow(errno);
    }

    // Close fdv
    for (int* p = fdv; p < fdp; ++p)
    {
        close(*p);
    }

    // if the method doesn't return anything including exceptions, we could simply return here.

    // wait for the reply. note the thread might receive another request by recursive call, etc.
    RpcHdr* hdr;
    int* fdmax;
    for (;;)
    {
        RpcStack stackBase;

        hdr = receiveMessage(&msg, fdv, fdmax, &s);
        if (hdr->cmd == RPC_REQ)
        {
            current.callLocal(reinterpret_cast<RpcReq*>(hdr), fdv, s);
            continue;
        }
        if (hdr->cmd == RPC_RES && hdr->tag == tag)
        {
            // restore response
            fdp = fdv;
            RpcRes* res(reinterpret_cast<RpcRes*>(hdr));
            if (res->exceptionCode)
            {
                while (fdp < fdmax)
                {
                    close(*fdp++);
                }
                esThrow(res->exceptionCode);
            }
            long long rc;
            switch (returnType.getType())
            {
            case Ent::SpecAny:  // XXX x86 specific
                rc = res->result.s64;
                break;
            case Ent::SpecBool:
            case Ent::SpecChar:
            case Ent::SpecWChar:
            case Ent::SpecS8:
            case Ent::SpecS16:
            case Ent::SpecS32:
            case Ent::SpecU8:
            case Ent::SpecU16:
            case Ent::SpecU32:
                rc = res->result.s32;
                break;
            case Ent::SpecS64:
            case Ent::SpecU64:
                rc = res->result.s64;
                break;
            case Ent::SpecF32:  // XXX
                retF32(res->result.f32);
                break;
            case Ent::SpecF64:  // XXX
                retF64(res->result.f64);
                break;
            case Ent::SpecString:
                rc = res->result.s32;
                if (0 < rc)
                {
                    memcpy(const_cast<void*>(rpcmsg.argv[1].ptr), res->getData(), rc + 1);
                }
                break;
            case Ent::SpecWString:
                rc = res->result.s32;
                if (0 < rc)
                {
                    memcpy(const_cast<void*>(rpcmsg.argv[1].ptr), res->getData(), sizeof(wchar_t) * (rc + 1));
                }
                break;
            case Ent::TypeSequence:
                rc = res->result.s32;
                memcpy(const_cast<void*>(rpcmsg.argv[1].ptr), res->getData(), returnType.getSize() * rc);
                break;
            case Ent::TypeInterface:
                iid = returnType.getInterface().getIid();
                // FALL THROUGH
            case Ent::SpecObject:
                if (const_cast<void*>(res->result.ptr))
                {
                    Capability* cap = static_cast<Capability*>(res->getData());
                    if (cap->check == 0 && (fdmax - fdp) == 1)
                    {
                        // Set cap->object to received fd number
                        cap->object = *fdp++;
                        // TODO Set exec on close to cap->object
                    }
#ifdef VERBOSE
                    printf(">> ");
                    cap->report();
#endif
                    rc = reinterpret_cast<long>(current.importObject(*cap, iid, false));
                }
                else
                {
                    rc = 0;
                }
                break;
            case Ent::TypeArray:
                memcpy(const_cast<void*>(rpcmsg.argv[1].ptr), res->getData(), returnType.getSize());
                rc = 0;
                break;
            case Ent::SpecVoid:
                rc = 0;
                break;
            }
            while (fdp < fdmax)
            {
                close(*fdp++);
            }
            return rc;
            break;
        }
    }
}

void* System::servant(void* param)
{
    struct msghdr msg;
    int fdv[8];
    int* fdmax;
    int s;

    epfd = (int) param;
    socketMap = new std::map<pid_t, int>;   // TODO clean up after thread termination
    RpcStack::init();
    for (;;)
    {
        RpcStack stackBase;
        RpcHdr* hdr = receiveMessage(&msg, fdv, fdmax, &s); // TODO check message length msg.msg_iov[0].iov_len
        if (hdr->cmd == RPC_REQ)
        {
            // Look up the exportedTable and invoke method internally
            ::current.callLocal(reinterpret_cast<RpcReq*>(hdr), fdv, s);
        }
        else
        {
            for (int* fdp = fdv; fdp < fdmax; ++fdp)
            {
                close(*fdp);
            }
        }
    }
}

}   // namespace

ICurrentProcess* System()
{
    return &current;
}
