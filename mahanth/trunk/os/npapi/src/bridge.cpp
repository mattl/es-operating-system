/*
 * Copyright 2008 Google Inc.
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

#include <sys/epoll.h>
#include <sys/types.h>

#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include <es/base/IStream.h>
#include <es/naming/IContext.h>
#include <es/util/ICanvasRenderingContext2D.h>

#include "bridge.h"
#include "monitor.h"

// #define VERBOSE

#ifndef VERBOSE
#define FPRINTF(...)    (__VA_ARGS__)
#else
#define FPRINTF(...)    fprintf(__VA_ARGS__)
#endif

namespace
{

__thread int rpctag;

NPObject* getCanvas(NPP npp)
{
    NPObject* window;
    NPN_GetValue(npp, NPNVWindowNPObject, &window);
    NPVariant docv;
    NPN_GetProperty(npp, window, NPN_GetStringIdentifier("document"), &docv);
    NPVariant strv;
    NPVariant canvasv;
    STRINGZ_TO_NPVARIANT("canvas", strv);
    NPN_Invoke(npp, NPVARIANT_TO_OBJECT(docv), NPN_GetStringIdentifier("getElementById"), &strv, 1, &canvasv);

    NPVariant contextv;
    STRINGZ_TO_NPVARIANT("2d", strv);
    NPN_Invoke(npp, NPVARIANT_TO_OBJECT(canvasv), NPN_GetStringIdentifier("getContext"), &strv, 1,
               &contextv);

    NPN_ReleaseVariantValue(&canvasv);
    NPN_ReleaseVariantValue(&docv);
    NPN_ReleaseObject(window);

    return NPVARIANT_TO_OBJECT(contextv);
}

}   // namespace

int Process::
exportObject(NPObject* object, const Guid& iid, es::Capability* cap, bool param)
{
    if (!object)
    {
        cap->pid = 0;
        cap->object = -1;
        cap->check = -1;
        return -1;
    }

    // If object has been exported, reuse the ExportedObject entry by incrementing ref
    ExportKey key(object, iid);
    int i = exportedTable.add(key);
    if (0 <= i)
    {
        NPN_RetainObject(object);

        cap->pid = getpid();
        cap->object = i;
        ExportedObject* exported = exportedTable.get(i);
        cap->check = exported->getCheck();
        exportedTable.put(i);
    }
    return i;
}

NPObject* Process::
importObject(const es::Capability& cap, const Guid& iid, bool param)
{
    if (cap.object < 0)
    {
        return 0;
    }

    if (cap.check == 0)
    {
        return 0;
    }

    ImportKey key(npp, cap, iid);
    int i = importedTable.add(key);
    if (0 <= i)
    {
        ImportedObject* imported = importedTable.get(i);
        if (!param)
        {
            ASSERT(imported);
            imported->addRef();
        }
        importedTable.put(i);
        return imported;
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

Process::
Process(NPP npp) :
    npp(npp),
    pid(-1),
    exportedTable(new(std::nothrow) NullMonitor),
    importedTable(new(std::nothrow) NullMonitor)
{
    // TODO: Set currentThread for plugin thread
}

Process::
~Process()
{
    // Close all the connections

    // Kill the client process
    kill(pid, SIGTERM);

    if (0 <= pid)
    {
        processMap->remove(pid);
    }
}

int Process::
start(const char* path, int argc, char* argn[], char* argv[])
{
    if (pid != -1)
    {
        return 0;
    }

    // export in, out, error, root, context to child
    cmd.cmd = es::CMD_FORK_RES;
    cmd.pid = getpid();
    exportObject(0, es::IStream::iid(), &cmd.in, false);
    exportObject(0, es::IStream::iid(), &cmd.out, false);
    exportObject(0, es::IStream::iid(), &cmd.error, false);
    exportObject(0, es::IContext::iid(), &cmd.current, false);  // should be NPNVWindowNPObject?
    exportObject(0, es::IContext::iid(), &cmd.root, false);     // should be NPNVPluginElementNPObject?

    // for test purpose only
    exportObject(getCanvas(npp), es::ICanvasRenderingContext2D::iid(), &cmd.document, false);

    cmd.report();

    int pfd[2];
    pipe(pfd);
    char token;

    pid = fork();
    if (pid)
    {
        // parent
        close(pfd[0]);

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

        if (0 <= pid)
        {
            processMap->add(pid, this);
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
        close(controlSocket);

        FPRINTF(stderr, "ppid: %d\n", getppid());

        /// TEST
        for (int i = 0; i < argc; ++i)
        {
            FPRINTF(stderr, "%d: %s %s\n", i, argn[i], argv[i]);
        }
        /// TEST END

        char* command = strdup("esjs shell.js");   // TODO: ???
        char* argv[32];
        int argc = 0;
        while (argc < 32)
        {
            argv[argc++] = command;
            char c;
            while ((c = *command))
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

        execv(argv[0], argv);
        exit(EXIT_FAILURE); // TODO: Plugin should detect the child process termination
    }
    return pid;
}

void Process::
accept(es::CmdChanReq* req)
{
    // Check ThreadCredential and if the thread is created already, just ad fd to its socketMap.
    std::map<es::ThreadCredential, Thread*>::iterator it = threadMap.find(req->tc);
    if (it == threadMap.end())
    {
        Thread* thread = new(std::nothrow) Thread(this, req->sockfd);
        if (thread)
        {
            threadMap[req->tc] = thread;
        }
    }
    else
    {
        Thread* thread = (*it).second;
        thread->accept(req->sockfd);
    }
    // client is not waiting in the current implementation - sendmsg(fd, CMD_CHAN_RES);
}

// Use NPN_PluginThreadAsyncCall() to invoke NPAPI functions. Do not call them directly.
void* Process::
focus(void* param)
{
    int fdv[8];
    int* fdmax;
    int s;

    currentThread = static_cast<Thread*>(param);
    es::RpcStack::init();
    for (;;)
    {
        es::RpcStack stackBase;
        es::RpcHdr* hdr = es::receiveMessage(currentThread->epfd, fdv, fdmax, &s);
        switch (hdr->cmd)
        {
        case es::RPC_REQ: {
            es::RpcReq* req = reinterpret_cast<es::RpcReq*>(hdr);
            ExportedObject* exported = currentThread->process->getExported(req->capability);
            if (!exported)
            {
                // TODO: send back exception code
                break;
            }

            std::map<int, int>::iterator it = currentThread->socketMap.find(hdr->pid);
            if (it == currentThread->socketMap.end())
            {
                currentThread->socketMap[hdr->pid] = s;
            }

            exported->invoke(req, fdv, fdmax, s);

            break;
        }
        default:
            for (int* fdp = fdv; fdp < fdmax; ++fdp)
            {
                close(*fdp);
            }
            break;
        }
    }
}
