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
#include <cairo-xlib.h>
#include <X11/Xlib.h>

#include <stdio.h>
#include <stdlib.h>

#define SIZEX 1024
#define SIZEY 768

void testCanvas2d(cairo_t* cairo);

int main(int argc, char* argv[])
{
    Display* display = XOpenDisplay(NULL);
    if (!display)
    {
        return EXIT_FAILURE;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    Window window = XCreateSimpleWindow(display, root, 1, 1, SIZEX, SIZEY, 0,
                              BlackPixel(display, screen), BlackPixel(display, screen));

    XStoreName(display, window, "WebKit test");
    XSelectInput(display, window, ExposureMask|ButtonPressMask);
    XMapWindow(display, window);

    cairo_surface_t* cs = cairo_xlib_surface_create(display, window, DefaultVisual(display, 0), SIZEX, SIZEY);
    for (;;)
    {
        XEvent event;
        XNextEvent(display, &event);
        if (event.type == Expose && event.xexpose.count < 1)
        {
            cairo_t* cairo = cairo_create(cs);
            cairo_rectangle(cairo, 0.0, 0.0, SIZEX, SIZEY);
            cairo_set_source_rgb(cairo, 0.0, 0.0, 1.0);
            cairo_fill(cairo);
            testCanvas2d(cairo);
            cairo_show_page(cairo);
            cairo_destroy(cairo);
        }
        else if (event.type == ButtonPress)
        {
            break;
        }
    }

    cairo_surface_destroy(cs);
    XCloseDisplay(display);
}
