/*
 * Copyright 2008 Chis Dan Ionut
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
#include <es/types.h>
#include <es/handle.h>
#include <es/base/IStream.h>
#include <es/base/IProcess.h>
#include <es/naming/IContext.h>
#include <es/device/ICursor.h>

#define BITS_PER_PIXEL 32
#define BPP (BITS_PER_PIXEL / 8)

u8 pixels[1024 * 768 * BPP];



extern es::CurrentProcess* System();

void fill(es::Stream* fb, u8 r, u8 g, u8 b)
{
    for (long offset = 0; offset < 1024 * 768 * BPP; offset += 1024 * 768)
    {
        for (long j = offset; j < 1024 * 768; j += BPP)
        {
            pixels[j] = b;
            pixels[j + 1] = g;
            pixels[j + 2] = r;
        }
        fb->write(pixels, 1024 * 768, offset);
    }
}

void fill(void* fb, u8 r, u8 g, u8 b)
{
    u32* fbp = static_cast<u32*>(fb);
    u32 color = (r << 16) | (g << 8) | b;
    for (int y = 0; y < 768; ++y) {
        for (int x = 0; x < 1024; ++x) {
            *fbp++ = color;
        }
    }
}

void pattern(es::Stream* fb)
{
    long offset;
    for (offset = 0; offset < 1024 * 256 * BPP; offset += BPP)
    {
        pixels[offset] = 255;
        pixels[offset + 1] = 0;
        pixels[offset + 2] = 0;
    }
    fb->write(pixels, 1024 * 256 * BPP, 0);

    for (; offset < 1024 * 512 * BPP; offset += BPP)
    {
        pixels[offset] = 0;
        pixels[offset + 1] = 255;
        pixels[offset + 2] = 0;
    }
    fb->write(pixels + 1024 * 256 * BPP, 1024 * 256 * BPP, 1024 * 256 * BPP);

    for (; offset < 1024 * 768 * BPP; offset += BPP)
    {
        pixels[offset] = 0;
        pixels[offset + 1] = 0;
        pixels[offset + 2] = 255;
    }
    fb->write(pixels + 1024 * 512 * BPP, 1024 * 256 * BPP, 1024 * 512 * BPP);
}

int main()
{
    Handle<es::Context> root = System()->getRoot();
    Handle<es::Stream> mouse(root->lookup("device/mouse"));
    Handle<es::Cursor> cursor(root->lookup("device/cursor"));
    Handle<es::Stream> framebuffer(root->lookup("device/framebuffer"));

    void* mapping = System()->map(0, framebuffer->getSize(),
                                  es::CurrentProcess::PROT_READ | es::CurrentProcess::PROT_WRITE,
                                  es::CurrentProcess::MAP_SHARED,
                                  Handle<es::Pageable>(framebuffer), 0);

    fill(mapping, 255, 0, 0);
#ifndef __es__
    esSleep(10000000);
#endif
    fill(mapping, 0, 255, 0);
#ifndef __es__
    esSleep(10000000);
#endif
    fill(mapping, 0, 0, 255);
#ifndef __es__
    esSleep(10000000);
#endif
    fill(mapping, 255, 255, 255);
#ifndef __es__
    esSleep(10000000);
#endif

    pattern(framebuffer);
#ifndef __es__
    esSleep(10000000);
#endif

    cursor->show();

    for (int y = 0; y < 768; ++y)
    {
        cursor->setPosition(512, y);
#ifndef __es__
        esSleep(10000);
#endif
    }

    for (int x = 0; x < 1024; ++x)
    {
        cursor->setPosition(x, 768 / 2);
#ifndef __es__
        esSleep(10000);
#endif
    }

    for (int y = 0; y < 768; ++y)
    {
        cursor->setPosition(1023, y);
#ifndef __es__
        esSleep(10000);
#endif
    }

    for (int x = 0; x < 1024; ++x)
    {
        cursor->setPosition(x, 767);
#ifndef __es__
        esSleep(10000);
#endif
    }

    System()->unmap(mapping, framebuffer->getSize());

    esReport("done.\n");    // for testing
    return 0;
}
