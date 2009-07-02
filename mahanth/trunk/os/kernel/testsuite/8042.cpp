/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

#include <es.h>
#include <es/handle.h>
#include <es/naming/IContext.h>
#include "core.h"

int main()
{
    Object* nameSpace;
    esInit(&nameSpace);

    Handle<es::Context> root(nameSpace);
    Handle<es::Stream> keyboard(root->lookup("device/keyboard"));
    Handle<es::Stream> mouse(root->lookup("device/mouse"));

    esReport("done.\n");    // for testing

    for (;;)
    {
        u8 buffer[8];
        long count;
        int i;

        count = keyboard->read(buffer, 8);
        if (1 < count)
        {
            esReport("kbd:");
            for (i = 0; i < count; ++i)
            {
                esReport(" %02x", buffer[i]);
            }
            esReport("\n");
        }

        count = mouse->read(buffer, 8);
        if (4 <= count && (buffer[0] || buffer[1] || buffer[2] || buffer[3]))
        {
            esReport("aux:");
            for (i = 0; i < count; ++i)
            {
                esReport(" %02x", buffer[i]);
            }
            esReport("\n");
        }

        esSleep(10000000 / 60);
    }

    esPanic(__FILE__, __LINE__, "done.\n");
}
