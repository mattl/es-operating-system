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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include <new>

#include <es/rpc.h>

#include "bridge.h"

int controlSocket = -1;
ProcessMap* processMap;

namespace
{

pthread_t controlThread;

void* control(void* param)
{
    for (;;)
    {
        es::CmdUnion cmd;

        ssize_t rc = es::receiveCommand(controlSocket, &cmd);
        if (rc == -1)
        {
            continue;
        }

        switch (cmd.cmd)
        {
        case es::CMD_CHAN_REQ:
            if (Process* process = processMap->get(cmd.chanReq.pid))
            {
                process->accept(&cmd.chanReq);
            }
            break;
        case es::CMD_CHAN_RES:
            break;
        case es::CMD_FORK_REQ:
            // Look up the corrensponding Process
            if (Process* process = processMap->get(cmd.forkReq.pid))
            {
                // Send CmdForkRes
                struct sockaddr_un sa;
                sendto(controlSocket, &process->getCmdForkRes(), sizeof(es::CmdForkRes),
                       MSG_DONTWAIT, es::getSocketAddress(cmd.forkReq.pid, &sa), sizeof sa);
            }
            break;
        case es::CMD_FORK_RES:
            break;
        }
    }
    return 0;
}

}   // namespace

char* NPP_GetMIMEDescription()
{
    return "application/es-npapi-plugin:.elf:ES NPAPI bridge Plugin";
}

// Invoked from NP_Initialize()
NPError NPP_Initialize()
{
    printf("NPP_Initialize %d\n", controlSocket);

    // NPP_Initialize seems to be called more than once
    if (controlSocket != -1)
    {
        return NPERR_NO_ERROR;
    }

    processMap = new(std::nothrow) ProcessMap;
    if (!processMap)
    {
        return NPERR_OUT_OF_MEMORY_ERROR;
    }

    controlSocket = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (controlSocket == -1)
    {
        perror("socket");
        delete processMap;
        return NPERR_GENERIC_ERROR;
    }

    struct sockaddr_un sa;
    if (bind(controlSocket, es::getSocketAddress(getpid(), &sa), sizeof sa) == -1)
    {
        perror("bind");
        delete processMap;
        close(controlSocket);
        return NPERR_GENERIC_ERROR;
    }

    if (pthread_create(&controlThread, NULL, control, 0) != 0)
    {
        perror("pthread_create");
        return NPERR_GENERIC_ERROR;
    }
    pthread_detach(controlThread);

    return NPERR_NO_ERROR;
}

// Invoked from NP_Shutdown()
void NPP_Shutdown()
{
    printf("NPP_Shutdown %d\n", controlSocket);

    if (0 <= controlSocket)
    {
        close(controlSocket);
        char sun_path[108]; // UNIX_PATH_MAX
        sprintf(sun_path, "/tmp/es-socket-%u", getpid());
        printf("unlink %s\n", sun_path);
        unlink(sun_path);
        controlSocket = -1;
    }

    pthread_cancel(controlThread);

    if (processMap)
    {
        delete processMap;
        processMap = NULL;
    }
}

NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16 mode,
                int16 argc, char* argn[], char* argv[],
                NPSavedData* saved)
{
    printf("%s %d\n", __func__, __LINE__);
    if (instance == NULL)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    Process* process = new(std::nothrow) Process(instance);
    if (process == NULL)
    {
        return NPERR_OUT_OF_MEMORY_ERROR;
    }
    instance->pdata = static_cast<void*>(process);
#if 0
    if (process->start("esjs", argc, argn, argv) <= 0)  // TODO: esjs??
    {
        delete process;
        return NPERR_GENERIC_ERROR;
    }
#endif
    return NPERR_NO_ERROR;
}

NPError NPP_Destroy(NPP instance, NPSavedData** save)
{
    printf("NPP_Destroy\n");
    if (instance == NULL)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    Process* process = static_cast<Process*>(instance->pdata);
    if (process != NULL)
    {
        delete process;
    }
    return NPERR_NO_ERROR;
}

NPError NPP_SetWindow(NPP instance, NPWindow* window)
{
    printf("NPP_SetWindow %p\n", window);
    if (instance == NULL)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    if (window == NULL)
    {
        return NPERR_GENERIC_ERROR;
    }
    if (instance->pdata == NULL)
    {
        return NPERR_GENERIC_ERROR;
    }
    return NPERR_NO_ERROR;
}

NPError NPP_GetValue(NPP instance, NPPVariable variable, void* value)
{
    printf("NPP_GetValue %d\n", variable);
    if (instance == NULL)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    Process* process = static_cast<Process*>(instance->pdata);
    if (process == NULL)
    {
        return NPERR_GENERIC_ERROR;
    }

    switch (variable)
    {
    case NPPVpluginNameString:
        *reinterpret_cast<const char**>(value) = "NPRuntimeTest";
        break;
    case NPPVpluginDescriptionString:
        *reinterpret_cast<const char**>(value) = "NPRuntime scriptability API test plugin";
        break;
    case NPPVpluginScriptableNPObject:
        *reinterpret_cast<NPObject**>(value) = process->getScriptableObject();
        if (*reinterpret_cast<NPObject**>(value))
        {
            return NPERR_GENERIC_ERROR;
        }
        // process->start("esjs", 0, NULL, NULL);   // for test only
        break;
    default:
        break;
    }
    return NPERR_NO_ERROR;
}

int16 NPP_HandleEvent(NPP instance, void* event)
{
    return 0;
}

NPObject* NPP_GetScriptableInstance(NPP instance)
{
    printf("NPP_GetScriptableInstance\n");
    if (instance == NULL)
    {
        return 0;
    }
    Process* process = static_cast<Process*>(instance->pdata);
    if (process)
    {
        return process->getScriptableObject();
    }
    return 0;
}

void NPP_StreamAsFile(NPP instance, NPStream* stream, const char* fname)
{
    printf("%s: %s\n", __func__, fname);

    if (instance == NULL)
    {
        return;
    }
    Process* process = static_cast<Process*>(instance->pdata);
    if (process == NULL)
    {
        return;
    }
    process->start("esjs", 0, NULL, NULL);   // for test only
}
