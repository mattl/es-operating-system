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
#include <es/device/IBeep.h>
#include "core.h"

unsigned notes[] =
{
    262,
    294,
    330,
    349,
    392,
    440,
    494,
    523
};

int main()
{
    Object* nameSpace;
    esInit(&nameSpace);

    Handle<es::Context> root(nameSpace);
    Handle<es::Beep> speaker = root->lookup("device/beep");

    for (int i = 0; i < 8; ++i)
    {
        speaker->setFrequency(notes[i]);
        speaker->beep();
        esSleep(10000000);
    }

    esReport("done.\n");
}
