/*
 * Copyright (c) 2007
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#include <string.h>
#include <cairo.h>
#include <es.h>
#include <es/handle.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>

using namespace es;

ICurrentProcess* System();

#define WIDTH   1024
#define HEIGHT  768

Handle<IPageable> framebuffer;
u8* framebufferPtr;

void init()
{
    Handle<IContext> root = System()->getRoot();
    framebuffer = root->lookup("device/framebuffer");
    long long size;
    size = framebuffer->getSize();
    framebufferPtr = (u8*) System()->map(0, size,
                                 ICurrentProcess::PROT_READ | ICurrentProcess::PROT_WRITE,
                                 ICurrentProcess::MAP_SHARED,
                                 framebuffer, 0);
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
        memmove(framebufferPtr + 4 * WIDTH * y,
                data + 4 * 120 * y,
                4 * 120);
    }
}
