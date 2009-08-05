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
#include <es/base/IProcess.h>
#include <es/base/IStream.h>

#include "html5_canvasrenderingcontext2d.h"

#define SIZEX 1024
#define SIZEY 768
   
extern es::CurrentProcess* System();
u8* framebufferPtr;

void testCanvas2d(cairo_t* cairo);
void figure(es::CanvasRenderingContext2D* canvas);

int main(int argc, char* argv[])
{
    Handle<es::Context> nameSpace = System()->getRoot();
    esReport("1.\n");

    Handle<es::Stream> framebuffer(nameSpace->lookup("device/framebuffer"));
    esReport("2.\n");
//    void* mapping = System()->map(0, framebuffer->getSize(),
//                                  es::CurrentProcess::PROT_READ | es::CurrentProcess::PROT_WRITE,
//                                  es::CurrentProcess::MAP_SHARED,
//                                  Handle<es::Pageable>(framebuffer), 0);
    framebufferPtr = (u8*) System()->map(0, framebuffer->getSize(),
                                  es::CurrentProcess::PROT_READ | es::CurrentProcess::PROT_WRITE,
                                  es::CurrentProcess::MAP_SHARED,
                                  Handle<es::Pageable>(framebuffer), 0);


    esReport("3.\n");

    // Register canvas
    cairo_surface_t* surface;
    esReport("4.\n");
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1024, 768);
    
    //int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, 1024);
    //unsigned char *data = malloc (stride * canvasInfo.height);

    //surface = cairo_image_surface_create(canvasInfo.format, canvasInfo.width, canvasInfo.height);
    //surface = cairo_image_surface_create_for_data(
    //    static_cast<u8*>(mapping), CAIRO_FORMAT_ARGB32, 1024, 768, stride);

    //surface = cairo_image_surface_create_for_data(
    //      static_cast<u8*>(mapping), CAIRO_FORMAT_ARGB32, 1024, 768,
    //      sizeof(u32) * 1024);

    
    cairo_t* cairo;
    esReport("create.\n");
    cairo = cairo_create(surface);
    ASSERT(cairo);

    esReport("Finished create canvas.\n");

    esReport("5.\n");
    //cairo_rectangle(cairo, 0.0, 0.0, 50, 50);
    //cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
    //cairo_fill(cairo);

    //cairo_translate (cairo, 10, 10);

/*
    cairo_scale (cairo, 1, 1);

    cairo_set_source_rgb (cairo, 0, 0, 0);
    cairo_move_to (cairo, 0, 0);
    cairo_line_to (cairo, 1, 1);
    cairo_move_to (cairo, 1, 0);
    cairo_line_to (cairo, 0, 1);
    cairo_set_line_width (cairo, 0.2);
    cairo_stroke (cairo);
 
    cairo_rectangle (cairo, 0, 0, 0.5, 0.5);
    cairo_set_source_rgba (cairo, 1, 0, 0, 0.80);
    cairo_fill (cairo);
*/
    u8* data = cairo_image_surface_get_data (surface);
/*
    for (int y = 0; y < 1024; ++y)
    {
        //framebuffer->write(data + 4 * 120 * y, 4 * 120, 4 * 1024 * y);
        memmove(framebufferPtr + 4 * 1024 * y,
                data + 4 * 1024 * y,
                4 * 768);
    }
*/
/*
    for (int y = 0; y < framebuffer->getSize(); ++y)
    {
        *framebufferPtr++ = *data++;
        *framebufferPtr++ = *data++;
        *framebufferPtr++ = *data++;
    }
*/
    esReport("6.\n");

    testCanvas2d(cairo);

    //u8* data = cairo_image_surface_get_data (surface);
    /*
    for (int y = 0; y < 1024; ++y)
    {
        //framebuffer->write(data + 4 * 120 * y, 4 * 120, 4 * 1024 * y);
        memmove(framebufferPtr + 4 * 1024 * y,
                data + 4 * 1024 * y,
                4 * 768);
    }
    */

    for (int y = 0; y < framebuffer->getSize(); ++y)
    {
        *framebufferPtr++ = *data++;
        *framebufferPtr++ = *data++;
        *framebufferPtr++ = *data++;
        data++;
    }

    esReport("7.\n");
    cairo_show_page(cairo);
    //cairo_destroy(cairo);

    //cairo_surface_destroy(surface);
    //System()->unmap(mapping, framebuffer->getSize());
    //System()->unmap(framebufferPtr, framebuffer->getSize());
    esReport("8.\n");

    esReport("quit canvas.\n");
}
