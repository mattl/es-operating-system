/*
 * Copyright 2008, 2009 Google Inc.
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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <es.h>
#include <es/handle.h>
#include <es/base/IProcess.h>
#include <es/naming/IBinding.h>
#include <es/naming/IContext.h>

extern es::CurrentProcess* System();

int main()
{
    printf("server: %d\n", getpid());

    Handle<es::Stream> output = System()->getOutput();
    output->write("hello!\n", 7);

    Handle<es::Context> nameSpace = System()->getRoot();
    Handle<es::Iterator> iterator = nameSpace->list("");
    while (iterator->hasNext())
    {
        char name[128];

        Handle<es::Binding> binding = iterator->next();
        if (binding->getName(name, sizeof name))
        {
            output->write(name, strlen(name));
            output->write("\n", 1);
        }
    }

    Handle<es::File> elfFile = nameSpace->lookup("file/client");
    ASSERT(elfFile);

    Handle<es::Process> process = es::Process::createInstance();
    ASSERT(process);

    process->setRoot(nameSpace);
    process->setCurrent(nameSpace);
    process->start(elfFile);

    int result = process->wait();
    printf("server: done. %d\n", result);
    return result;
}
