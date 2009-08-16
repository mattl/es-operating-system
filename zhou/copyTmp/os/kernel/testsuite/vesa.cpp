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
#include "vesa.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void fill(es::Stream* fb, u8 r, u8 g, u8 b)
{
    u8 pixel[3];

    for (long offset = 0; offset < 1024 * 768 * 3; offset += 3)
    {
        pixel[0] = b;
        pixel[1] = g;
        pixel[2] = r;
        fb->write(pixel, 3, offset);
    }
}

void pattern(es::Stream* fb)
{
    u8 pixel[3];

    long offset;
    for (offset = 0; offset < 1024 * 768 * 1; offset += 3)
    {
        pixel[0] = 255;
        pixel[1] = 0;
        pixel[2] = 0;
        fb->write(pixel, 3, offset);
    }
    for (; offset < 1024 * 768 * 2; offset += 3)
    {
        pixel[0] = 0;
        pixel[1] = 255;
        pixel[2] = 0;
        fb->write(pixel, 3, offset);
    }
    for (; offset < 1024 * 768 * 3; offset += 3)
    {
        pixel[0] = 0;
        pixel[1] = 0;
        pixel[2] = 255;
        fb->write(pixel, 3, offset);
    }
}

int main()
{
    Object* nameSpace;
    esInit(&nameSpace);

    Handle<es::Context> root(nameSpace);
    Handle<es::Stream> mouse(root->lookup("device/mouse"));
    Handle<es::Cursor> cursor(root->lookup("device/cursor"));
    Handle<es::Stream> framebuffer(root->lookup("device/framebuffer"));
    Handle<es::Pageable> pageable(framebuffer);
    TEST(pageable);

    fill(framebuffer, 255, 0, 0);
    fill(framebuffer, 0, 255, 0);
    fill(framebuffer, 0, 0, 255);
    fill(framebuffer, 255, 255, 255);

    pattern(framebuffer);
    cursor->show();

    for (int y = 0; y < 768; ++y)
    {
        cursor->setPosition(512, y);
    }

    for (int x = 0; x < 1024; ++x)
    {
        cursor->setPosition(x, 768 / 2);
    }

    for (int y = 0; y < 768; ++y)
    {
        cursor->setPosition(1023, y);
    }

    for (int x = 0; x < 1024; ++x)
    {
        cursor->setPosition(x, 767);
    }

    long long size;
    size = framebuffer->getSize();
    for (long long offset(0); offset < size; offset += 4096)
    {
        unsigned long pte = pageable->get(offset);
        esReport("%llx: %lx\n", offset, pte);
    }

    esReport("done.\n");    // for testing

    cursor->setPosition(1024 / 2, 768 / 2);
    for (;;)
    {
        s8 buffer[4];
        long count;
        int i;

        count = mouse->read(buffer, 4);
        if (4 <= count && (buffer[1] || buffer[2]))
        {
            cursor->move(buffer[1], -buffer[2]);
        }
        esSleep(10000000 / 60);
    }
}
