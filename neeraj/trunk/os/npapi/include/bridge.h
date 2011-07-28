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

#ifndef GOOGLE_ES_NPAPI_BRIDGE_H_INCLUDED
#define GOOGLE_ES_NPAPI_BRIDGE_H_INCLUDED

#include "npapi.h"
#include "npruntime.h"
#include "npupp.h"

#include <map>

#include <es/objectTable.h>
#include <es/reflect.h>
#include <es/rpc.h>
#include <es/uuid.h>

#include <es/base/IMonitor.h>

#include "exportedObject.h"
#include "importedObject.h"

class Process
{
    static const int MaxExport = 100;
    static const int MaxImport = 100;

public:
    class Thread
    {
        Process* process;
        pthread_t thread;
        int epfd;
        std::map<int, int> socketMap;  // process id -> socket descriptor
    public:
        Thread(Process* process, int s);
        ~Thread();
        int accept(int s);

        Process* getProcess() const
        {
            return process;
        }

        NPP getNpp() const
        {
            return process->getNpp();
        }

        friend class Process;
    };

private:
    NPP npp;
    int pid;
    ObjectTable<ExportKey, ExportedObject, MaxExport> exportedTable;
    ObjectTable<ImportKey, ImportedObject, MaxImport> importedTable;
    es::CmdForkRes cmd;
    std::map<es::ThreadCredential, Thread*> threadMap;

public:
    Process(NPP npp);
    ~Process();

    NPP getNpp() const
    {
        return npp;
    }

    int exportObject(NPObject* object, const Guid& iid, es::Capability* cap, bool param);
    NPObject* importObject(const es::Capability& cap, const Guid& iid, bool param);

    ExportedObject* getExported(es::Capability& cap)
    {
        if (cap.pid != getpid())
        {
            return 0;
        }
        ExportedObject* exported = exportedTable.get(cap.object);
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

    ExportedObject* getExported(int index)
    {
        return exportedTable.get(index);
    }

    int putExported(int index)
    {
        return exportedTable.put(index);
    }

    int start(const char* path, int argc, char* argn[], char* argv[]);

    NPObject* getScriptableObject()
    {
        return 0;
    }

    const es::CmdForkRes& getCmdForkRes() const
    {
        return cmd;
    }

    void accept(es::CmdChanReq* req);
    static void* focus(void* param);
};

class ProcessMap
{
    std::map<int, Process*> map;

public:
    void add(int pid, Process* process)
    {
        map.insert(std::pair<pid_t, Process*>(pid, process));
    }

    Process* get(int pid)
    {
        std::map<pid_t, Process*>::iterator it = map.find(pid);
        if (it != map.end())
        {
            return (*it).second;
        }
        return 0;
    }

    void remove(int pid)
    {
        std::map<pid_t, Process*>::iterator it = map.find(pid);
        if (it != map.end())
        {
            map.erase(it);
        }
    }
};

extern int controlSocket;
extern ProcessMap* processMap;
extern __thread Process::Thread* currentThread;

#endif  // GOOGLE_ES_NPAPI_BRIDGE_H_INCLUDED
