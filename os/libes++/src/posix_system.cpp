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
#include <pthread.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>
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
#include <es/capability.h>
#include <es/classFactory.h>
#include <es/clsid.h>
#include <es/dateTime.h>
#include <es/handle.h>
#include <es/interlocked.h>
#include <es/objectTable.h>
#include <es/ref.h>
#include <es/reflect.h>
#include <es/rpc.h>
#include <es/timeSpan.h>
#include <es/variant.h>
#include <es/base/IProcess.h>
#include <es/util/ICanvasRenderingContext2D.h>

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

//
// Misc.
//

long long callRemote(const Capability& cap, unsigned methodNumber, va_list ap, Reflect::Method& method, Variant* variant);
u64 getRandom();

typedef long long (*Method)(void* self, ...);

void printGuid(const Guid& guid)
{
    fprintf(stderr, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
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
            check(::getRandom()),
            doRelease(false)
        {
        }

        ~Exported()
        {
            // Do release if necessary on objectTable
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

        CmdForkReq forkReq = {
            CMD_FORK_REQ,
            getpid()
        };
        CmdUnion cmd;
        ssize_t rc = sendto(sockfd, &forkReq, sizeof forkReq, MSG_DONTWAIT, getSocketAddress(getppid(), &sa), sizeof sa);
        if (rc != sizeof forkReq)
        {
            cmd.forkRes.in.object = -1;
            cmd.forkRes.out.object = -1;
            cmd.forkRes.error.object = -1;
            cmd.forkRes.root.object = -1;
            cmd.forkRes.current.object = -1;
            cmd.forkRes.document.object = -1;
        }
        else
        {
            ssize_t rc = receiveCommand(sockfd, &cmd);
            if (rc == -1 || cmd.cmd != CMD_FORK_RES)
            {
                ::exit(EXIT_FAILURE);
            }
            cmd.forkRes.report();
        }

        // Import root and current
        if (0 <= cmd.forkRes.root.object)
        {
            esInitThread();
            root = static_cast<IContext*>(importObject(cmd.forkRes.root, IContext::iid(), false));
        }
        else
        {
            // Root
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

            int fd = open(".", O_RDONLY);
            Dir* file = new Dir(fd, "");
            root->bind("file", static_cast<IContext*>(file));
            file->release();
        }

        // Import in, out, error
        if (0 <= cmd.forkRes.in.object)
        {
            in = static_cast<IStream*>(importObject(cmd.forkRes.in, IStream::iid(), false));
        }
        else
        {
            in = new Stream(0);
#ifdef __linux__
            struct termio tty;
            ioctl(0, TCGETA, &tty);
            tty.c_lflag &= ~(ICANON|ECHO);
            ioctl(0, TCSETAF, &tty);
#endif
        }
        if (0 <= cmd.forkRes.out.object)
        {
            out = static_cast<IStream*>(importObject(cmd.forkRes.out, IStream::iid(), false));
        }
        else
        {
            out = new Stream(1);
        }
        if (0 <= cmd.forkRes.error.object)
        {
            error = static_cast<IStream*>(importObject(cmd.forkRes.error, IStream::iid(), false));
        }
        else
        {
            error = new Stream(2);
        }

        if (0 <= cmd.forkRes.current.object)
        {
            current = static_cast<IContext*>(importObject(cmd.forkRes.current, IContext::iid(), false));
        }
        else
        {
            setCurrent(root);
        }

        // hack
        if (0 <= cmd.forkRes.document.object)
        {
            try
            {
                ICanvasRenderingContext2D* canvas = static_cast<ICanvasRenderingContext2D*>(importObject(cmd.forkRes.document, ICanvasRenderingContext2D::iid(), false));
                Handle<IContext> device = root->lookup("device");
                device->bind("canvas", canvas);
            }
            catch (...)
            {
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
            char sun_path[108]; // UNIX_PATH_MAX
            sprintf(sun_path, "/tmp/es-socket-%u", getpid());
            printf("unlink %s\n", sun_path);
            unlink(sun_path);
            }

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

    void* map(void* start, long long length, unsigned int prot, unsigned int flags, IPageable* pageable, long long offset)
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

    void unmap(void* start, long long length)
    {
        munmap(const_cast<void*>(start), length);
    }

    ICurrentThread* currentThread()
    {
    }

    // IThread* esCreateThread(void* (*start)(void* param), void* param)
    IThread* createThread(void* start, void* param) // [check] start must be a function pointer.
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
        if (!object)
        {
            cap->pid = 0;
            cap->object = -1;
            cap->check = -1;
            return -1;
        }

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
        if (exported->getCheck() != cap.check)
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
            unsigned int count;

            switch (methodNumber - baseMethodCount)
            {
            case 1: // addRef
                count = exported->addRef();
                if (1 != count)
                {
                    res.result = Variant(static_cast<uint32_t>(count));
                    return 0;
                }
                exportedTable.get(hdr->capability.object);
                break;
            case 2: // release
                count = exported->release();
                exportedTable.put(hdr->capability.object);
                if (0 <= count) // TODO: ==?
                {
                    exportedTable.put(hdr->capability.object);
                }
                res.result = Variant(static_cast<uint32_t>(count));
                // TODO should return?
                break;
            }
        }

        Guid iid = IInterface::iid();
        Method** object = reinterpret_cast<Method**>(exported->object);
        int argc = hdr->paramCount;
        Variant* argv = hdr->getArgv();
        Variant* argp = argv;

        // Set this
        *argp++ = Variant(exported->object);

        // Reserve space from rpcStack to store result
        void* resultPtr = 0;
        Reflect::Type returnType = method.getReturnType();
        switch (returnType.getType())
        {
        case Ent::SpecVariant:
            resultSize = std::max(static_cast<int32_t>(sizeof(Capability)), static_cast<int32_t>(argp[1]));
            resultPtr = RpcStack::alloc(resultSize);
            *argp++ = Variant(reinterpret_cast<intptr_t>(resultPtr));
            ++argp;
            break;
        case Ent::SpecString:
            // int op(char* buf, int len, ...);
            resultPtr = RpcStack::alloc(static_cast<int32_t>(argp[1]));
            *argp++ = Variant(reinterpret_cast<intptr_t>(resultPtr));
            ++argp;
            break;
        case Ent::TypeSequence:
            // int op(xxx* buf, int len, ...);
            resultPtr = RpcStack::alloc(returnType.getSize() * static_cast<int32_t>(argp[1]));
            *argp++ = Variant(reinterpret_cast<intptr_t>(resultPtr));
            ++argp;
            break;
        case Ent::SpecUuid:
        case Ent::TypeStructure:
            // void op(struct* buf, ...);
            resultSize = returnType.getSize();
            resultPtr = RpcStack::alloc(resultSize);
            *argp++ = Variant(reinterpret_cast<intptr_t>(resultPtr));
            break;
        case Ent::TypeArray:
            // void op(xxx[x] buf, ...);
            resultSize = returnType.getSize();
            resultPtr = RpcStack::alloc(resultSize);
            *argp++ = Variant(reinterpret_cast<intptr_t>(resultPtr));
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
            case Ent::SpecVariant:
                switch (argp->getType())
                {
                case Variant::TypeString:
                    if (static_cast<const char*>(*argp))
                    {
                        *argp = Variant(reinterpret_cast<const char*>(data));
                        size = strlen(reinterpret_cast<const char*>(data)) + 1;
                        data += size;
                    }
                    break;
                case Variant::TypeObject:
                    if (static_cast<IInterface*>(*argp))
                    {
                        // Import object
                        Capability* cap = reinterpret_cast<Capability*>(data);
                        if (cap->check == 0)
                        {
                            cap->object = *fdv++;   // TODO check range
                        }
                        IInterface* object = importObject(*cap, iid, true); // TODO false?
                        *argp = Variant(object);
                        data += sizeof(Capability);
                    }
                    break;
                }
                argp->makeVariant();
                break;
            case Ent::TypeSequence:
                // xxx* buf, int len, ...
                size = type.getSize() * static_cast<uint32_t>(argp[1]);
                *argp++ = Variant(reinterpret_cast<intptr_t>(data));
                data += size;
                break;
            case Ent::SpecString:
                if (static_cast<const char*>(*argp))
                {
                    *argp = Variant(reinterpret_cast<const char*>(data));
                    size = strlen(reinterpret_cast<const char*>(data)) + 1;
                    data += size;
                }
                break;
            case Ent::SpecUuid:
                if (static_cast<const Guid*>(*argp))
                {
                    *argp = Variant(reinterpret_cast<const Guid*>(data));
                    size = sizeof(Guid);
                    data += size;
                    iid = *static_cast<const Guid*>(*argp);
                }
                break;
            case Ent::TypeStructure:
            case Ent::TypeArray:
                size = type.getSize();
                *argp = Variant(reinterpret_cast<intptr_t>(data));
                data += size;
                break;
            case Ent::TypeInterface:
                iid = type.getInterface().getIid();
                // FALL THROUGH
            case Ent::SpecObject:
                if (static_cast<IInterface*>(*argp))
                {
                    // Import object
                    Capability* cap = reinterpret_cast<Capability*>(data);
                    if (cap->check == 0)
                    {
                        cap->object = *fdv++;   // TODO check range
                    }
                    IInterface* object = importObject(*cap, iid, true); // TODO false?
                    *argp = Variant(object);
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
        case Ent::SpecVariant:
            res.result = apply(argc, argv, (Variant (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecBool:
            res.result = apply(argc, argv, (bool (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecChar:
        case Ent::SpecS8:
        case Ent::SpecU8:
            res.result = apply(argc, argv, (uint8_t (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecWChar:
        case Ent::SpecS16:
            res.result = apply(argc, argv, (int16_t (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecU16:
            res.result = apply(argc, argv, (uint16_t (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecS32:
            res.result = apply(argc, argv, (int32_t (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecAny:  // XXX x86 specific
        case Ent::SpecU32:
            res.result = apply(argc, argv, (uint32_t (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecS64:
            res.result = apply(argc, argv, (int64_t (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecU64:
            res.result = apply(argc, argv, (uint64_t (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecF32:
            res.result = apply(argc, argv, (float (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecF64:
            res.result = apply(argc, argv, (double (*)()) ((*object)[methodNumber]));
            break;
        case Ent::SpecString:
            res.result = apply(argc, argv, (int32_t (*)()) ((*object)[methodNumber]));
            if (0 <= static_cast<int32_t>(res.result)) {
                resultSize = 1 + static_cast<int32_t>(res.result);
            }
            else
            {
                resultSize = 0;
            }
            break;
        case Ent::TypeSequence:
            res.result = apply(argc, argv, (int32_t (*)()) ((*object)[methodNumber]));
            resultSize = sizeof(returnType.getSize() * static_cast<int32_t>(res.result));   // XXX maybe set just the # of elements
            break;
        case Ent::TypeInterface:
            iid = returnType.getInterface().getIid();
            // FALL THROUGH
        case Ent::SpecObject:
            res.result = apply(argc, argv, (IInterface* (*)()) ((*object)[methodNumber]));
            break;
        case Ent::TypeArray:
        case Ent::SpecVoid:
            apply(argc, argv, (int32_t (*)()) ((*object)[methodNumber]));
            break;
        }

        // Export object
        if (res.result.getType() == Variant::TypeObject && static_cast<IInterface*>(res.result))
        {
            Capability* cap = static_cast<Capability*>(resultPtr);
            exportObject(res.result, iid, cap, false);   // TODO error check, TODO true??
            if (cap->check == 0)
            {
                *fdp++ = cap->object;
            }
#ifdef VERBOSE
            printf("<< ");
            cap->report();
#endif
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

    long long callRemote(int interfaceNumber, int methodNumber, va_list ap, Variant* variant)
    {
        long long result = 0;
        int error = 0;

        Imported* imported = importedTable.get(interfaceNumber);
        if (!imported)
        {
            throw SystemException<EBADF>();
        }

        // Determine the type of interface and which method is being invoked.
        Reflect::Interface interface;
        try
        {
            interface = getInterface(imported->iid);
        }
        catch (...)
        {
        }

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

        result = ::callRemote(imported->capability, methodNumber, ap, method, variant);

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

    int getControlSocket()
    {
        return sockfd;
    }
};

System current __attribute__((init_priority(1001)));    // After InterfaceStore

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
        ::current.exportObject(0, IInterface::iid(), &cmd.document, false);
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
            close(::current.getControlSocket());

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
        ssize_t rc = receiveCommand(::current.sockfd, &cmd);
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
                struct sockaddr_un sa;
                ssize_t rc = sendto(::current.sockfd, &child->getCmdForkRes(), sizeof(CmdForkRes),
                                    MSG_DONTWAIT, getSocketAddress(cmd.forkReq.pid, &sa), sizeof sa);
                if (rc == sizeof(CmdForkRes))
                {
                    ::current.removeChild(cmd.forkReq.pid);
                }
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

long long callRemote(void* self, void* base, int methodNumber, va_list ap)
{
    unsigned interfaceNumber = static_cast<void**>(self) - static_cast<void**>(base);
    Variant* variant = 0;
    if (MAX_IMPORT < interfaceNumber)
    {
        // self is a pointer to a Variant value for the method that returns a Variant.
        variant = reinterpret_cast<Variant*>(self);
        self = va_arg(ap, void*);
        interfaceNumber = static_cast<void**>(self) - static_cast<void**>(base);
        ASSERT(interfaceNumber < MAX_IMPORT);
    }
    return current.callRemote(interfaceNumber, methodNumber, ap, variant);
}

long long callRemote(const Capability& cap, unsigned methodNumber, va_list ap, Reflect::Method& method, Variant* variant)
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
        Variant argv[9];
    } rpcmsg = { RPC_REQ, tag, getpid(), cap, methodNumber };
    Variant* argp = rpcmsg.argv;
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
    *argp++ = Variant(static_cast<intptr_t>(0));  // to be filled by the server

    Reflect::Type returnType = method.getReturnType();
    switch (returnType.getType())
    {
    case Ent::SpecVariant:
        // Variant op(void* buf, int len, ...);
        // FALL THROUGH
    case Ent::TypeSequence:
    case Ent::SpecString:
        // int op(xxx* buf, int len, ...);
        *argp++ = Variant(reinterpret_cast<intptr_t>(va_arg(ap, void*)));
        *argp++ = Variant(va_arg(ap, int32_t));
        break;
    case Ent::SpecUuid:
    case Ent::TypeStructure:
        // void op(struct* buf, ...);
        *argp++ = Variant(reinterpret_cast<intptr_t>(va_arg(ap, void*)));
        break;
    case Ent::TypeArray:
        // void op(xxx[x] buf, ...);
        *argp++ = Variant(reinterpret_cast<intptr_t>(va_arg(ap, void*)));
        break;
    }

    // TODO: padding
    for (int i = 0; i < method.getParameterCount(); ++i, ++argp)
    {
        Reflect::Parameter param(method.getParameter(i));
        Reflect::Type type(param.getType());
        assert(param.isInput());

        switch (type.getType())
        {
        case Ent::SpecVariant:
            // Variant variant, ...
            *argp = Variant(va_arg(ap, VariantBase));
            switch (argp->getType())
            {
            case Variant::TypeString:
                if (static_cast<const char*>(*argp))
                {
                    iop->iov_base = const_cast<char*>(static_cast<const char*>(*argp));
                    iop->iov_len = strlen(static_cast<const char*>(iop->iov_base)) + 1;
                    ++iop;
                }
                break;
            case Variant::TypeObject:
                if (static_cast<IInterface*>(*argp))
                {
                    IInterface* object = static_cast<IInterface*>(*argp);
                    current.exportObject(object, iid, capp, true);  // TODO check error
                    iop->iov_base = capp;
                    iop->iov_len = sizeof(Capability);
                    if (capp->check == 0)
                    {
                        *fdp++ = capp->object;
                    }
                    ++capp;
                    ++iop;
                }
                break;
            }
            break;
        case Ent::TypeSequence:
            // xxx* buf, int len, ...
            iop->iov_base = va_arg(ap, void*);
            *argp++ = Variant(reinterpret_cast<intptr_t>(iop->iov_base));
            *argp = Variant(va_arg(ap, int32_t));
            iop->iov_len = type.getSize() * static_cast<int32_t>(*argp);
            ++iop;
            break;
        case Ent::SpecString:
            iop->iov_base = va_arg(ap, char*);
            *argp = Variant(reinterpret_cast<const char*>(iop->iov_base));
            if (iop->iov_base)
            {
                iop->iov_len = strlen(static_cast<const char*>(iop->iov_base)) + 1;
                ++iop;
            }
            break;
        case Ent::SpecUuid:
            iop->iov_base = va_arg(ap, Guid*);
            if (iop->iov_base)
            {
                iop->iov_len = sizeof(Guid);
                iid = *static_cast<const Guid*>(iop->iov_base);
                *argp = Variant(reinterpret_cast<Guid*>(iop->iov_base));
                ++iop;
            } else {
                *argp = Variant(reinterpret_cast<Guid*>(0));
            }
            break;
        case Ent::TypeStructure:
            iop->iov_base = va_arg(ap, void*);
            *argp = Variant(reinterpret_cast<intptr_t>(iop->iov_base));
            iop->iov_len = type.getSize();
            ++iop;
            break;
        case Ent::TypeArray:
            iop->iov_base = va_arg(ap, void*);
            *argp = Variant(reinterpret_cast<intptr_t>(iop->iov_base));
            iop->iov_len = type.getSize();
            ++iop;
            break;
        case Ent::TypeInterface:
            iid = type.getInterface().getIid();
            // FALL THROUGH
        case Ent::SpecObject: {
            IInterface* object = va_arg(ap, IInterface*);
            *argp = Variant(object);
            if (object)
            {
                current.exportObject(object, iid, capp, true);  // TODO check error
                iop->iov_base = capp;
                iop->iov_len = sizeof(Capability);
                if (capp->check == 0)
                {
                    *fdp++ = capp->object;
                }
                ++capp;
                ++iop;
            }
            break;
        }
        case Ent::SpecBool:
            *argp = Variant(static_cast<bool>(va_arg(ap, int)));
            break;
        case Ent::SpecAny:
            *argp = Variant(reinterpret_cast<intptr_t>(va_arg(ap, void*)));
            break;
        case Ent::SpecS16:
            *argp = Variant(static_cast<int16_t>(va_arg(ap, int)));
            break;
        case Ent::SpecS32:
            *argp = Variant(va_arg(ap, int32_t));
            break;
        case Ent::SpecS8:
        case Ent::SpecU8:
            *argp = Variant(static_cast<uint8_t>(va_arg(ap, int)));
            break;
        case Ent::SpecU16:
            *argp = Variant(static_cast<uint16_t>(va_arg(ap, int)));
            break;
        case Ent::SpecU32:
            *argp = Variant(va_arg(ap, uint32_t));
            break;
        case Ent::SpecS64:
            *argp = Variant(va_arg(ap, int64_t));
            break;
        case Ent::SpecU64:
            *argp = Variant(va_arg(ap, uint64_t));
            break;
        case Ent::SpecF32:
            {
                // XXX works on X86 only probably
                uint32_t value = va_arg(ap, uint32_t);
                *argp = Variant(*reinterpret_cast<float*>(&value));
            }
            break;
        case Ent::SpecF64:
            *argp = Variant(va_arg(ap, double));
            break;
        default:
            break;
        }
    }

    int argc = argp - rpcmsg.argv;
    rpcmsg.req.paramCount = argc;
    iov[0].iov_base = &rpcmsg;
    iov[0].iov_len = sizeof(RpcReq) + sizeof(Variant) * argc;

#ifdef VERBOSE
    printf("Send RpcReq: %d %d\n", s, argc);
    esDump(&rpcmsg, sizeof(RpcReq));
    for (int i = 0; i < argc; ++i) {
        esDump((char*) &rpcmsg + sizeof(RpcReq) + sizeof(Variant) * i, sizeof(Variant));
    }
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

        hdr = receiveMessage(epfd, fdv, fdmax, &s);
        std::map<pid_t, int>::iterator it = socketMap->find(hdr->pid);
        if (it == socketMap->end())
        {
            (*socketMap)[hdr->pid] = s;
        }
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
            case Ent::SpecVariant:
                switch (res->result.getType())
                {
                case Variant::TypeString:
                    memmove(reinterpret_cast<void*>(static_cast<intptr_t>(rpcmsg.argv[1])), res->getData(),
                            static_cast<int32_t>(rpcmsg.argv[2]));
                    break;
                case Variant::TypeObject:
                    if (static_cast<IInterface*>(res->result))
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
                        res->result = current.importObject(*cap, iid, false);
                    }
                    else
                    {
                        res->result = Variant(static_cast<IInterface*>(0));
                    }
                    break;
                }
                assert(variant);
                *variant = res->result;
                res->result = Variant(reinterpret_cast<intptr_t>(variant));
                break;
            case Ent::SpecString:
                rc = static_cast<int32_t>(res->result);
                if (0 < rc)
                {
                    memmove(reinterpret_cast<void*>(static_cast<intptr_t>(rpcmsg.argv[1])), res->getData(),
                            rc + 1);
                }
                break;
            case Ent::TypeSequence:
                rc = static_cast<int32_t>(res->result);
                if (0 < rc)
                {
                    memmove(reinterpret_cast<void*>(static_cast<intptr_t>(rpcmsg.argv[1])), res->getData(),
                            returnType.getSize() * rc);
                }
                break;
            case Ent::TypeInterface:
                iid = returnType.getInterface().getIid();
                // FALL THROUGH
            case Ent::SpecObject:
                if (static_cast<IInterface*>(res->result))
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
                    res->result = current.importObject(*cap, iid, false);
                }
                else
                {
                    res->result = Variant(static_cast<IInterface*>(0));
                }
                break;
            case Ent::TypeArray:
                memcpy(reinterpret_cast<void*>(static_cast<intptr_t>(rpcmsg.argv[1])), res->getData(),
                       returnType.getSize());
                break;
            case Ent::SpecVoid:
                break;
            }
            while (fdp < fdmax)
            {
                close(*fdp++);
            }
            return evaluate(res->result);
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

    epfd = reinterpret_cast<intptr_t>(param);
    socketMap = new std::map<pid_t, int>;   // TODO clean up after thread termination
    RpcStack::init();
    for (;;)
    {
        RpcStack stackBase;
        RpcHdr* hdr = receiveMessage(epfd, fdv, fdmax, &s); // TODO check message length msg.msg_iov[0].iov_len
        std::map<pid_t, int>::iterator it = socketMap->find(hdr->pid);
        if (it == socketMap->end())
        {
            (*socketMap)[hdr->pid] = s;
        }
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
