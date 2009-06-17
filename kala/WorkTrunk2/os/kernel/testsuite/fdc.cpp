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

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/naming/IContext.h>
#include "8237a.h"
#include "core.h"
#include "fdc.h"

u8 sec[512];

int main()
{
    Object* nameSpace;
    esInit(&nameSpace);

    Handle<es::Context> root(nameSpace);
    Handle<es::Stream> floppy = root->lookup("device/floppy");

    memset(sec, 0, 512);

    floppy->setPosition(0x2600);
    floppy->read(sec, 512);

    esDump(sec, 512);

    es::DiskManagement::Geometry geo;
    Handle<es::DiskManagement> dm(floppy);
    dm->getGeometry(&geo);
    esReport("%d %d %d %d %lld\n",
             geo.heads,
             geo.cylinders,
             geo.sectorsPerTrack,
             geo.bytesPerSector,
             geo.diskSize);

    esSleep(50000000);      // for motor stop

    esReport("done.\n");    // for testing
}
