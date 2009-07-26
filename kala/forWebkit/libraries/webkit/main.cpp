/*
 * Copyright (c) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cairo.h>
#include <string>
#include <math.h>
#include <es.h>
#include <es/handle.h>
#include <es/exception.h>
#include <es/formatter.h>
#include <es/interlocked.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/ring.h>
#include <es/synchronized.h>
#include <es/types.h>
#include <es/usage.h>
#include <es/utf.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <es/base/IService.h>

namespace
{
    Interlocked registered = 0;

    struct CanvasInfo
    {
        int x; // top-left
        int y; // top-left
        cairo_format_t format;
        int width;
        int height;
    };
};

#define SIZEX 1024
#define SIZEY 768
   
extern es::CurrentProcess* System();

void testCanvas2d(cairo_t* cairo);

int main(int argc, char* argv[])
{
    Handle<es::Context> nameSpace = System()->getRoot();

    Handle<es::Stream> framebuffer(nameSpace->lookup("device/framebuffer"));
    void* mapping = System()->map(0, framebuffer->getSize(),
                                  es::CurrentProcess::PROT_READ | es::CurrentProcess::PROT_WRITE,
                                  es::CurrentProcess::MAP_SHARED,
                                  Handle<es::Pageable>(framebuffer), 0);

    // Register canvas
    cairo_surface_t* surface;
    CanvasInfo canvasInfo;
    canvasInfo.x = 0;
    canvasInfo.y = 0;
    canvasInfo.width = 1024;
    canvasInfo.height = 768;
    canvasInfo.format = CAIRO_FORMAT_ARGB32;    // or CAIRO_FORMAT_RGB24

    // surface = cairo_image_surface_create(canvasInfo.format, canvasInfo.width, canvasInfo.height);
    /*surface = cairo_image_surface_create_for_data(
        static_cast<u8*>(mapping), canvasInfo.format , canvasInfo.width, canvasInfo.height,
        sizeof(u32) * canvasInfo.width);
    CanvasRenderingContext2D_Impl* canvas = new CanvasRenderingContext2D_Impl(surface, canvasInfo.width, canvasInfo.height);
    ASSERT(canvas);
    Handle<es::Context> device = nameSpace->lookup("device");
    device->bind("canvas", static_cast<es::CanvasRenderingContext2D*>(canvas));
    ASSERT(nameSpace->lookup("device/canvas"));*/

    cairo_t* cairo = cairo_create(surface);
    cairo_rectangle(cairo, 0.0, 0.0, SIZEX, SIZEY);
    cairo_set_source_rgb(cairo, 0.0, 0.0, 1.0);
    cairo_fill(cairo);
    testCanvas2d(cairo);
    cairo_show_page(cairo);
    cairo_destroy(cairo);

    cairo_surface_destroy(surface);

    System()->unmap(mapping, framebuffer->getSize());

    esReport("quit canvas.\n");
}
