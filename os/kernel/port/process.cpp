/*
 * Copyright 2011 Esrille Inc.
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

#include <string.h>
#include <es.h>
#include "core.h"
#include "elfFile.h"
#include "process.h"
#include "interfaceStore.h"

// #define VERBOSE

extern es::CurrentProcess* esCurrentProcess();

Swap* Process::swap;
Zero* Process::zero;

void Process::
initialize()
{
    zero = new Zero;
    es::Cache* cache = es::Cache::createInstance(zero);
    swap = new Swap(dynamic_cast<Cache*>(cache));
}

void Process::
dump()
{
    Map* map;
    const void* start = 0;

    Map::List::Iterator iter = mapList.begin();
    while ((map = iter.next()))
    {
        esReport("%p-%p (%lx) %x %x\n",
                 map->start, map->end, map->length,
                 map->prot, map->flags);
        ASSERT(map->start < map->end);
        ASSERT(start <= map->start);
        start = map->start;
    }
}

Map* Process::
lookup(const void* addr)
{
    Map* map;

    Map::List::Iterator iter = mapList.begin();
    while ((map = iter.next()))
    {
        if (map->end <= addr)
        {
            continue;
        }
        if (map->start <= addr)
        {
            return map;
        }
        else
        {
            return 0;
        }
    }
    return 0;
}

bool Process::
isValid(const void* start, long long length, bool write)
{
    const void* end;

    end = static_cast<const u8*>(start) + length;
    if (start < end)    // check wrap around
    {
        Map* map;
        Map::List::Iterator iter = mapList.begin();
        while ((map = iter.next()))
        {
            if (start < map->start)
            {
                break;
            }
            if (start < map->end)
            {
                if (write && !(map->prot & es::CurrentProcess::PROT_WRITE))
                {
                    return false;
                }
                if (map->end < end)
                {
                    start = map->end;
                    continue;
                }
                return true;
            }
        }
    }
    return false;
}

void* Process::
map(void* start, long long length, unsigned prot, unsigned flags,
    es::Pageable* pageable, long long offset)
{
    Monitor::Synchronized method(monitor);

    long long size = length;
    if (pageable)
    {
        size = pageable->getSize();    // XXX size might change later
        size -= offset;
    }
    return map(start, length, prot, flags, pageable, offset, size, USER_MIN, USER_MAX);
}

void* Process::
map(void* start, long long length, unsigned prot, unsigned flags,
    es::Pageable* pageable, long long offset, long long size /* in pageable object */,
    void* min, void* max)
{
    Map::List::Iterator iter = mapList.begin();
    Map* map;
    void* end;

    if (Page::pageOffset(offset) != Page::pageOffset((unsigned long) start))
    {
        return 0;
    }
    if (length < size)
    {
        size = length;
    }
    start = Page::trunc(start);
    length += Page::pageOffset(offset);
    size += Page::pageOffset(offset);
    offset = Page::pageBase(offset);

    if (flags & es::CurrentProcess::MAP_FIXED)
    {
        if (start < min)
        {
            return 0;
        }
        end = static_cast<u8*>(start) + length;
        if (max < end || end <= start)
        {
            return 0;
        }
        while ((map = iter.next()))
        {
            if (end <= map->start)
            {
                iter.previous();
                break;
            }
            if (start < map->end)
            {
                return 0;
            }
        }
    }
    else
    {
        if (start < min)
        {
            start = min;
        }
        end = static_cast<u8*>(start) + length;
        while ((map = iter.next()))
        {
            if (end <= map->start)
            {
                iter.previous();
                break;
            }
            if (start < map->end)
            {
                start = const_cast<void*>(map->end);
                end = static_cast<u8*>(start) + length;
            }
        }
        if (max < end || end <= start)
        {
            return 0;
        }
    }

    if (!pageable)
    {
        // Just use swap file
        offset = 0;
        size = 0;
    }

    map = new Map(this, start, end, size, prot, flags, pageable, offset);
    if (map)
    {
        iter.add(map);
    }
    return const_cast<void*>(map->start);
}

void Process::
unmap(void* start, long long length)
{
    Monitor::Synchronized method(monitor);

    const void* end;
    Map* map;

    end = static_cast<const u8*>(start) + length;
    start = Page::trunc(start);
    end = Page::round(end);
    if (end <= start)   // check wrap around
    {
        return;
    }

    if (getCurrentProcess() == this)
    {
        load(); // flush TLB
    }

    Map::List::Iterator iter = mapList.begin();
    while ((map = iter.next()))
    {
        if (end <= map->start)
        {
            break;
        }
        if (map->end <= start)
        {
            continue;
        }

        if (start <= map->start && map->end <= end)
        {
            mmu->unset(map->start, map->length, map->pageable, map->offset);
            iter.remove();
            delete map;
            continue;
        }
        if (map->start < start)
        {
            mmu->unset(start, (u8*) map->end - (u8*) start, map->pageable,
                       map->offset + ((u8*) start - (u8*) map->start));
            map->end = start;
        }
        if (end < map->end)
        {
            mmu->unset(end, (u8*) map->end - (u8*) end, map->pageable,
                       map->offset + ((u8*) end - (u8*) map->start));
            map->start = end;
        }
        map->length = (u32*) map->end - (u32*) map->start;
    }
}

void* Process::
setBreak(long long increment)
{
    Monitor::Synchronized method(monitor);

    if (increment == 0)
    {
        return end;
    }

    Map* map;
    Map::List::Iterator iter = mapList.begin();
    while ((map = iter.next()))
    {
        if (map->end < end)
        {
            continue;
        }
        if (map->start <= end)
        {
            break;
        }
        else
        {
            return end;
        }
    }

    // Set map->end to newend if possible
    void* newend = (u8*) end + increment;
    if (0 < increment)
    {
        // Grow the data region
        if (newend < end || USER_MAX < newend)
        {
            return end;
        }

        Map* next(iter.next());
        if (next && next->start <= newend)
        {
            return end;
        }
    }
    else
    {
        // Shrink the data region
        if (end < newend)
        {
            return end;
        }

        if (newend < (u8*) map->start + map->length)
        {
            return end;
        }

        // XXX release swap pages
    }

    map->end = Page::round(newend);

    // sbrk() returns a pointer to the *start* of the new area.
    void* from = end;
    end = newend;
    return from;
}

bool Process::
trace(bool on)
{
    bool prev(log);
    log = on;
    return prev;
}

int Process::
validityFault(const void* addr, u32 error)
{
    Monitor::Synchronized method(monitor);

    addr = Page::trunc(addr);
    Map* map = lookup(addr);
    if (!map)
    {
        // XXX Let Core raise an exception to the current process.
        return -1;
    }

    Page* page;
    long long pos = map->getPosition(addr);
    u32 pte = mmu->get(addr);

    if (pte & Page::PTEVALID)
    {
        // The fault might have been resolved while locking the monitor.
        return 0;
    }

    if (!(map->flags & es::CurrentProcess::MAP_PRIVATE))
    {
        page = 0;
        pte = map->pageable->get(pos);
    }
    else if (pte & Page::PTEPRIVATE)
    {
        page = swap->restore(Page::pageBase(pte));
        if (page)
        {
            pte = Page::PTEVALID | Page::PTEPRIVATE;
        }
    }
    else if (map->length <= map->getOffset(addr))
    {
        page = swap->get();
        pte = Page::PTEVALID | Page::PTEPRIVATE;
    }
    else
    {
        page = 0;
        pte = map->pageable->get(pos);

        if (map->length < map->getOffset(Page::trunc(addr)) + Page::SIZE)
        {
            // Zero fill the tail portion of the page
            page = swap->get();
            if (!page)
            {
                // XXX Let Core raise an exception to the process.
                map->pageable->put(pos, 0);
                return -1;
            }
            memmove(page->getPointer(), mmu->getPointer(pte), Page::pageOffset(map->length));
            map->pageable->put(pos, 0);
            pte = Page::PTEVALID | Page::PTEPRIVATE;
        }
    }
    if (!(pte & Page::PTEVALID))
    {
        // Let Core raise an exception to the process.
        return -1;
    }

    mmu->set(addr, page, pte | Page::PTEUSER);
    return 0;
}

int Process::
protectionFault(const void* addr, u32 error)
{
    Monitor::Synchronized method(monitor);

    addr = Page::trunc(addr);
    Map* map = lookup(addr);
    if (!map)
    {
        // XXX Let Core raise an exception to the process.
        return -1;
    }

    if (error & Page::PTEWRITE)
    {
        if (!(map->prot & es::CurrentProcess::PROT_WRITE))
        {
            // XXX Let Core raise an exception to the process.
            return -1;
        }

        Page* page(0);
        unsigned long pte(0);
        long long pos(map->getPosition(addr));

        if (!(map->flags & es::CurrentProcess::MAP_PRIVATE))
        {
            pte = mmu->get(addr);
            if ((pte & (Page::PTEWRITE | Page::PTEVALID)) == (Page::PTEWRITE | Page::PTEVALID))
            {
                // The fault might have been resolved while locking the monitor.
                return 0;
            }
        }
        else
        {
            // Copy-on-write
            u32 prev = mmu->get(addr);
            ASSERT(prev & Page::PTEVALID);
            if ((prev & (Page::PTEWRITE | Page::PTEVALID)) == (Page::PTEWRITE | Page::PTEVALID))
            {
                // The fault might have been resolved while locking the monitor.
                return 0;
            }
            if (!(prev & Page::PTEPRIVATE))
            {
                page = swap->get();
                if (!page)
                {
                    // XXX Let Core raise an exception to the process.
                    // map->pageable->put(pos, 0);
                    return -1;
                }
                memmove(page->getPointer(), mmu->getPointer(prev), Page::SIZE);
                // map->pageable->put(pos, 0);
            }
            else
            {
                page = PageTable::lookup(Page::pageBase(prev));
                if (page->addRef() <= 2)
                {
                    page->release();
                }
                else
                {
                    Page* copy = swap->get();
                    if (!copy)
                    {
                        // XXX Let Core raise an exception to the process.
                        page->release();
                        return -1;
                    }
                    memmove(copy->getPointer(), page->getPointer(), Page::SIZE);
                    page->release();
                    page->release();
                    page = copy;
                }
            }
            pte |= Page::PTEPRIVATE;
        }

        mmu->set(addr, page, pte | Page::PTEVALID | Page::PTEUSER | Page::PTEWRITE);
    }
    return 0;
}

Process::
Process() :
    end(0),
    exitValue(0),
    startup(0),
    tlsImage(0),
    tlsImageSize(0),
    tlsSize(0),
    threadCount(0),
    root(0),
    current(0),
    in(0),
    out(0),
    error(0),
    log(false),
    upcallCount(0)
{
    es::Cache* cache = es::Cache::createInstance(zero);
    mmu = new Mmu(dynamic_cast<Cache*>(cache));
    ASSERT(mmu);

    syscallTable[0].set(esCurrentProcess(), es::CurrentProcess::iid(), true);

    Process* current(Process::getCurrentProcess());
    if (current)
    {
        setRoot(current->root);
        setInput(current->in);
        setOutput(current->out);
        setError(current->error);
    }
}

Process::
~Process()
{
#ifdef VERBOSE
    esReport("Process::~Process %p\n", this);
#endif

    setInput(0);
    setOutput(0);
    setError(0);
    setRoot(0);

    for (SyscallProxy* proxy(syscallTable);
         proxy < &syscallTable[INTERFACE_POINTER_MAX];
         ++proxy)
    {
        proxy->addRef();
        while (0 < proxy->release())
            ;
   }

    while (!upcallList.isEmpty())
    {
        upcallCount.decrement();
        UpcallRecord* record(upcallList.removeFirst());
        delete record;
    }
    ASSERT(upcallCount == 0);

    ASSERT(threadList.isEmpty());

    unmap(USER_MIN, static_cast<u8*>(USER_MAX) - static_cast<u8*>(USER_MIN));
    ASSERT(mmu);
    delete mmu;
}

void Process::
load()
{
    unsigned x = Core::splHi();
    Core* core = Core::getCurrentCore();
    ASSERT(core);
    core->currentProc = this;
    ASSERT(mmu);
    mmu->load();
    Core::splX(x);
}

// XXX if this interface has been registered already, should return the slot
// that has been assigned for the interface. However, the reference count of
// the interface pointer must also be adjusted to do this.
int Process::
set(SyscallProxy* table, void* interface, const char* iid, bool used)
{
    Monitor::Synchronized method(monitor);

    for (SyscallProxy* proxy(table);
         proxy < &table[INTERFACE_POINTER_MAX];
         ++proxy)
    {
        if (proxy->set(interface, iid, used))
        {
#ifdef VERBOSE
            esReport("Process::set(%p, %s) : %d;\n",
                     interface, iid, proxy - table);
#endif
            return proxy - table;
        }
    }
    esReport("Ins. interface pointer table.\n");
    return -1;
}

void Process::
// setStartup(void (*startup)(void* (*start)(void* param), void* param)) // [check] startup must be a function pointer.
setStartup(void* startup)
{
    typedef void (*Startup)(void* (*start)(void* param), void* param); // [check]
    this->startup = reinterpret_cast<Startup>(startup);
}

void* Process::
ast(void* param)
{
    Thread* thread(Thread::getCurrentThread());
    Process* process(thread->process);
    if (process->threadCount == 1)  // The default thread?
    {
        // Push arguments now as the process finally becomes the current one.

        // Set up TLS.
        void* tls = thread->tls(process->tlsSize, process->tlsAlign);
#ifdef VERBOSE
        esReport("ast: %p \n", tls);
#endif  // VERBOSE
        memmove(tls, process->tlsImage, process->tlsImageSize);
        memset((u8*) tls + process->tlsImageSize, 0, process->tlsSize - process->tlsImageSize);

        // Copy-out the argument.
        Ureg* ureg(static_cast<Ureg*>(thread->param));
        char* argument = (char*) &ureg[1];
        thread->setArguments(++argument);
    }

    static_cast<Ureg*>(param)->load();
    // NOT REACHED HERE
    return 0;   // lint
}

Thread* Process::
createThread(const unsigned stackSize)
{
    // Map a user stack
    void* userStack(static_cast<u8*>(USER_MAX) - ((threadCount + upcallCount + 1) * stackSize));
    userStack = map(userStack, stackSize - Page::SIZE,
                    es::CurrentProcess::PROT_READ | es::CurrentProcess::PROT_WRITE,
                    es::CurrentProcess::MAP_PRIVATE, 0, 0);
    if (!userStack)
    {
        return 0;
    }

    u8* stack = new u8[16*1024];
    if (!stack)
    {
        unmap(userStack, stackSize - Page::SIZE);
        return 0;
    }
    Ureg* ureg = reinterpret_cast<Ureg*>(stack);
    memset(ureg, 0, sizeof(Ureg));
    ureg->gs = Core::TCBSEL;
    ureg->fs = ureg->es = ureg->ds = ureg->ss = Core::UDATASEL;
    ureg->cs = Core::UCODESEL;
    ureg->eflags = 0x0202;  // IF
    ureg->esp = reinterpret_cast<unsigned>(static_cast<u8*>(userStack) + stackSize - Page::SIZE);

    Thread* thread = new Thread(ast,                // thread function
                                ureg,               // argument to thread function
                                es::Thread::Normal,    // priority
                                stack,              // stack
                                16*1024);           // stack size
    if (!thread)
    {
        unmap(userStack, stackSize - Page::SIZE);
        delete[] stack;
        return 0;
    }

    // Add this thread to this process.
    ++threadCount;
    thread->process = this;
    thread->userStack = userStack;
    threadList.addLast(thread);
    addRef();
    return thread;
}

void Process::
detach(Thread* thread)
{
    Monitor::Synchronized method(monitor);

    if (thread->userStack)
    {
        const unsigned stackSize = 2*1024*1024;
        unmap(thread->userStack, stackSize - Page::SIZE);
        thread->userStack = 0;
    }

    threadList.remove(thread);
    --threadCount;
    waitPoint.wakeup();

    release();
}

es::Thread* Process::
createThread(void* (*start)(void* param), void* param)
{
    Monitor::Synchronized method(monitor);

    if (threadCount <= 0)   // Process has been terminated
    {
        return 0;
    }

    const unsigned stackSize = 2*1024*1024;
    Thread* thread(createThread(stackSize));
    if (!thread)
    {
        return 0;
    }

    void* tls = thread->tls(tlsSize, tlsAlign);
    memmove(tls, tlsImage, tlsImageSize);
    memset((u8*) tls + tlsImageSize, 0, tlsSize - tlsImageSize);
    if (!startup)
    {
        thread->push(reinterpret_cast<unsigned>(param));
        thread->push(0);
        thread->entry(reinterpret_cast<unsigned>(start));
    }
    else
    {
        thread->push(reinterpret_cast<unsigned>(param));
        thread->push(reinterpret_cast<unsigned>(start));
        thread->push(0);
        thread->entry(reinterpret_cast<unsigned>(startup));
    }
    return thread;
}

void Process::
start()
{
}

void Process::
start(es::File* file)
{
    start(file, 0);
}

void Process::
start(es::File* file, const char* argument)
{
    Monitor::Synchronized method(monitor);

    // XXX Check no elf file is set yet.

    const unsigned stackSize = 2*1024*1024;
    Thread* thread(createThread(stackSize));
    ASSERT(thread);
    syscallTable[1].set(thread, es::Thread::iid(), true);   // just for reference counting

    Elf elf(file);

    if (elf.getType() != ET_EXEC)
    {
        esReport("Process::%s - the specified file is not an executable file.\n", __func__);
        return;
    }

    Handle<es::Pageable> pageable(file->getPageable());
    if (!pageable)
    {
        esReport("Process::%s - the specified file is not mappable file.\n", __func__);
        return;
    }

    for (int i(0); i < elf.getPhnum(); ++i)
    {
        Elf32_Phdr phdr;

        if (!elf.getPhdr(i, &phdr))
        {
            return;
        }

        switch (phdr.p_type)
        {
        case PT_TLS:
            // Make spece for TCB
#ifdef VERBOSE
            Elf::dumpPhdr(&phdr);
#endif  // VERBOSE
            tlsImage = (void*) phdr.p_vaddr;
            tlsImageSize = phdr.p_filesz;
            tlsSize = phdr.p_memsz;
            tlsAlign = phdr.p_align;
            break;
        case PT_LOAD:
            // Map PT_LOAD segment
            unsigned prot(es::CurrentProcess::PROT_NONE);
            if (phdr.p_flags & PF_X)
            {
                prot |= es::CurrentProcess::PROT_EXEC;
            }
            if (phdr.p_flags & PF_W)
            {
                prot |= es::CurrentProcess::PROT_WRITE;
            }
            if (phdr.p_flags & PF_R)
            {
                prot |= es::CurrentProcess::PROT_READ;
            }
            if (prot == es::CurrentProcess::PROT_NONE)
            {
                continue;
            }

            unsigned flags(es::CurrentProcess::MAP_FIXED);
            if (phdr.p_flags & PF_W)
            {
                flags |= es::CurrentProcess::MAP_PRIVATE;
            }
            else
            {
                flags |= es::CurrentProcess::MAP_SHARED;
            }

            if (phdr.p_flags & PF_W)
            {
                if ((unsigned long) end < phdr.p_vaddr + phdr.p_memsz)
                {
                    end = (void*) (phdr.p_vaddr + phdr.p_memsz);
                }
            }

            void* addr = map((void*) phdr.p_vaddr, phdr.p_memsz, prot, flags,
                             pageable, phdr.p_offset,
                             phdr.p_filesz, USER_MIN, USER_MAX);
            break;
        }
    }

    char name[32];
    file->getName(name, sizeof name);
    esReport("start %p %s from %p\n", this, name, elf.getEntry());

#ifdef VERBOSE
    dump();
    esReport("break: %p\n", end);
#endif // VERBOSE

    // Copy-in the argument.
    Ureg* ureg(static_cast<Ureg*>(thread->param));
    char* args = (char*) &ureg[1];
    *args++ = '\0'; // sentinel
    if (argument)
    {
        size_t len = strlen(argument);
        memmove(args, argument, len + 1);   // XXX over run check
    }

    thread->entry(elf.getEntry());
    thread->start();
}

int Process::
condWait(int)
{
    return hasExited();
}

int Process::
wait()
{
    DelegateTemplate<Process> d(this, &Process::condWait);
    waitPoint.sleep(&d);
}

int Process::
getExitValue()
{
    return exitValue;
}

bool Process::
hasExited()
{
    return (threadCount == 0) ? true : false;
}

void Process::
exit(int status)
{
    Monitor::Synchronized method(monitor);

    exitValue = status;

    // Cancel all the threads.
    Thread* thread;
    List<Thread, &Thread::linkProcess>::Iterator iter = threadList.begin();
    while ((thread = iter.next()))
    {
        if (thread == Thread::getCurrentThread())
        {
            continue;
        }
        thread->setCancelState(es::CurrentThread::CANCEL_ENABLE);
        thread->setCancelType(es::CurrentThread::CANCEL_DEFERRED);
        thread->cancel();
    }

    thread = Thread::getCurrentThread();
    thread->exit(0);
    // NOT REACHED HERE

    ASSERT(0);
}

void Process::
kill()
{
    if (getCurrentProcess() == this)
    {
        exit(1);
        // NOT REACHED HERE
    }

    // Cancel all the threads.
    Monitor::Synchronized method(monitor);
    Thread* thread;
    List<Thread, &Thread::linkProcess>::Iterator iter = threadList.begin();
    while ((thread = iter.next()))
    {
        ASSERT(thread != Thread::getCurrentThread());
        thread->setCancelState(es::CurrentThread::CANCEL_ENABLE);
        thread->setCancelType(es::CurrentThread::CANCEL_DEFERRED);
        thread->cancel();
    }
}

es::Context* Process::
getRoot()
{
    Monitor::Synchronized method(monitor);

    if (root)
    {
        root->addRef();
    }
    return root;
}

void Process::
setRoot(es::Context* root)
{
    Monitor::Synchronized method(monitor);

    es::Context* prev(this->root);
    if (root)
    {
        root->addRef();
    }
    this->root = root;
    if (prev)
    {
        prev->release();
    }
}

es::Context* Process::
getCurrent()
{
    Monitor::Synchronized method(monitor);

    if (current)
    {
        current->addRef();
    }
    return current;
}

void Process::
setCurrent(es::Context* current)
{
    Monitor::Synchronized method(monitor);

    es::Context* prev(this->current);
    if (current)
    {
        current->addRef();
    }
    this->current = current;
    if (prev)
    {
        prev->release();
    }
}

es::Stream* Process::
getInput()
{
    Monitor::Synchronized method(monitor);

    if (in)
    {
        in->addRef();
    }
    return in;
}

void Process::
setInput(es::Stream* in)
{
    Monitor::Synchronized method(monitor);

    es::Stream* prev(this->in);
    if (in)
    {
        in->addRef();
    }
    this->in = in;
    if (prev)
    {
        prev->release();
    }
}

es::Stream* Process::
getOutput()
{
    Monitor::Synchronized method(monitor);

    if (out)
    {
        out->addRef();
    }
    return out;
}

void Process::
setOutput(es::Stream* out)
{
    Monitor::Synchronized method(monitor);

    es::Stream* prev(this->out);
    if (out)
    {
        out->addRef();
    }
    this->out = out;
    if (prev)
    {
        prev->release();
    }
}

es::Stream* Process::
getError()
{
    Monitor::Synchronized method(monitor);

    if (error)
    {
        error->addRef();
    }
    return error;
}

void Process::
setError(es::Stream* error)
{
    Monitor::Synchronized method(monitor);

    es::Stream* prev(this->error);
    if (error)
    {
        error->addRef();
    }
    this->error = error;
    if (prev)
    {
        prev->release();
    }
}

Object* Process::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Process*>(this);
    }
    else if (strcmp(riid, es::Process::iid()) == 0)
    {
        objectPtr = static_cast<es::Process*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Process::
addRef()
{
    return ref.addRef();
}

unsigned int Process::
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

Process* Process::
getCurrentProcess()
{
    unsigned x = Core::splHi();
    Core* core = Core::getCurrentCore();
    Process* process = core->currentProc;
    Core::splX(x);
    return process;
}

int Process::
read(void* dst, int count, long long offset)
{
    u8* addr(reinterpret_cast<u8*>(offset));
    int len;
    int n;
    unsigned long pageOffset(Page::pageOffset(offset));
    for (len = 0;
         len < count;
         len += n, addr += n, dst = (u8*) dst + n)
    {
        if (validityFault(addr, es::CurrentProcess::PROT_READ) < 0)
        {
            break;
        }
        unsigned long pte = mmu->get(addr);
        if (!pte)
        {
            break;
        }
        u8* src = static_cast<u8*>(mmu->getPointer(pte));
        if (!src)
        {
            break;
        }
        n = Page::SIZE - pageOffset;
        if (count - len < n)
        {
            n = count - len;
        }
        memmove(dst, src + pageOffset, n);
        pageOffset = 0;
    }
    return len;
}

int Process::
write(const void* src, int count, long long offset)
{
    u8* addr(reinterpret_cast<u8*>(offset));
    int len;
    int n;
    unsigned long pageOffset(Page::pageOffset(offset));
    for (len = 0;
         len < count;
         len += n, addr += n, src = (u8*) src + n)
    {
        if (validityFault(addr, es::CurrentProcess::PROT_READ) < 0)
        {
            break;
        }
        if (protectionFault(addr, es::CurrentProcess::PROT_WRITE) < 0)
        {
            break;
        }
        unsigned long pte = mmu->get(addr);
        if (!pte)
        {
            break;
        }
        u8* dst = static_cast<u8*>(mmu->getPointer(pte));
        if (!dst)
        {
            break;
        }
        n = Page::SIZE - pageOffset;
        if (count - len < n)
        {
            n = count - len;
        }
        memmove(dst + pageOffset, src, n);
        pageOffset = 0;
    }
    return len;
}

es::Process* Process::Constructor::createInstance()
{
    return new Process;
}

Object* Process::Constructor::queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Process::Constructor::iid()) == 0)
    {
        objectPtr = static_cast<es::Process::Constructor*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::Process::Constructor*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Process::Constructor::addRef()
{
    return 1;
}

unsigned int Process::Constructor::release()
{
    return 1;
}

void Process::
initializeConstructor()
{
    static Constructor constructor;
    es::Process::setConstructor(&constructor);
}
