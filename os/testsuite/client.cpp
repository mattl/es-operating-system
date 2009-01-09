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

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <es.h>
#include <es/handle.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <es/naming/IContext.h>

using namespace es;

extern ICurrentProcess* System();

int main()
{
    printf("client: %d\n", getpid());

    int stop = 0;
    while (stop)
    {
        fprintf(stderr, ".");
        sleep(1);
    }

    Handle<IStream> output = System()->getOutput();
    output->write("hello!\n", 7);

    Handle<IContext> nameSpace = System()->getRoot();


    Handle<IIterator> iterator = nameSpace->list("");
    while (iterator->hasNext())
    {
        char name[128];

        Handle<IBinding> binding = iterator->next();
        int len = binding->getName(name, sizeof name);
        name[len++] = '\n';
        output->write(name, len);
    }
    return 0;
}
