/*
 * Copyright 2008 Chis Dan Ionut
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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
#include <es/types.h>
#include <es/handle.h>
#include <es/base/IStream.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>

#include <cairo/cairo.h>

#define WIDTH   1024
#define HEIGHT  768



extern es::CurrentProcess* System();

Handle<es::Stream> fb;

void init()
{
    Handle<es::Context> root = System()->getRoot();
    fb = root->lookup("device/framebuffer");
}

int main(int argc, char* argv[])
{
    init();

    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 120, 120);
    cr = cairo_create (surface);

    cairo_translate (cr, 10, 10);
    cairo_scale (cr, 100, 100);

    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_move_to (cr, 0, 0);
    cairo_line_to (cr, 1, 1);
    cairo_move_to (cr, 1, 0);
    cairo_line_to (cr, 0, 1);
    cairo_set_line_width (cr, 0.2);
    cairo_stroke (cr);

    cairo_rectangle (cr, 0, 0, 0.5, 0.5);
    cairo_set_source_rgba (cr, 1, 0, 0, 0.80);
    cairo_fill (cr);

    cairo_rectangle (cr, 0, 0.5, 0.5, 0.5);
    cairo_set_source_rgba (cr, 0, 1, 0, 0.60);
    cairo_fill (cr);

    cairo_rectangle (cr, 0.5, 0, 0.5, 0.5);
    cairo_set_source_rgba (cr, 0, 0, 1, 0.40);
    cairo_fill (cr);

    u8* data = cairo_image_surface_get_data (surface);
    for (int y = 0; y < 120; ++y)
    {
     // memmove(framebufferPtr + 4 * WIDTH * y,
     //            data + 4 * 120 * y,
     //           4 * 120);
        fb->write(data + 4 * 120 * y, 4 * 120, 4 * WIDTH * y);
     }

#ifndef __es__
     esSleep(10000000);
#endif
}
