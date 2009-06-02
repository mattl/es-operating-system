/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

#include <string>
#include <math.h>
#include <stdlib.h>
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
#include "canvas2d.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

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

es::CurrentProcess* System();

void transform(es::CanvasRenderingContext2D* canvas)
{
   int i = 0;
   char buf[40];
   float PI = 3.14159;
   float sinT = sin(PI/6);
   float cosT = cos(PI/6);
   canvas->translate(200, 200);

   float c = 0;
   for (i; i <= 12; i++) {
        c = floor(255 / 12 * i);
	sprintf(buf,"\"rgb(%f,%f,%f)\"",c,c,c);
	esReport("%s \n",buf);
	canvas->setFillStyle(buf);
        canvas->fillRect(0, 0, 100, 10);
        canvas->transform(cosT,sinT, -sinT, cosT, 0, 0);
   }
			        
   canvas->setTransform(-1, 0, 0, 1, 200, 200);
   canvas->setFillStyle("rgba(255, 128, 255, 0.5)");
   canvas->fillRect(0, 50, 100, 100);
}

// C++ version of the figure script
void figure(html5::CanvasRenderingContext2D* canvas)
{
    // Bar graph
    float top = 50.0f;
    float bottom = 250.0f;

    float width = 20.0f;
    float height;
    float x;
    float y;

    canvas->setStrokeStyle("rgb(0, 0, 0)");

    height = 100.0f;
    x = 100.0f;
    canvas->setLineWidth(3);
    canvas->setFillStyle("rgb(255, 0, 0)");

    canvas->fillRect(x, bottom - height, width, height);

    x += 50.0f;
    height = 200.0f;
    canvas->setFillStyle("rgb(0, 255, 0)");
    canvas->fillRect(x, bottom - height, width, height);

    x += 50.0f;
    height = 80.0f;
    canvas->setFillStyle("rgb(0, 0, 255)");
    canvas->fillRect(x, bottom - height, width, height);

    x += 50.0f;
    height = 50.0f;
    canvas->setFillStyle("rgb(255, 255, 0)");
    canvas->fillRect(x, bottom - height, width, height);

    x += 50.0f;
    height = 30.0f;
    canvas->setFillStyle("rgb(0, 255, 255)");
    canvas->fillRect(x, bottom - height, width, height);

    canvas->moveTo(80.0f, top);
    canvas->lineTo(80.0f, bottom);
    canvas->lineTo(350.0f, bottom);
    canvas->setLineWidth(4);
    canvas->stroke();

    // Circle graph
    float r = 100.0f;  // radius
    float cx = 180.0f; // center
    float cy = 450.0f; // center
    // angle
    float s = 270.0f/180.0f;
    float e = 120.0f/180.0f;

    canvas->setFillStyle("rgb(255, 0, 0)");
    canvas->beginPath();
    canvas->arc(cx, cy, r, M_PI * s, M_PI * e, 0);
    canvas->lineTo(cx, cy);
    canvas->closePath();
    canvas->fill();

    s = e;
    e = 240.0f/180.0f;
    canvas->setFillStyle("rgb(0, 255, 0)");
    canvas->beginPath();
    canvas->arc(cx, cy, r, M_PI * s, M_PI * e, 0);
    canvas->lineTo(cx, cy);
    canvas->closePath();
    canvas->fill();

    s = e;
    e = 260.0f/180.0f;
    canvas->setFillStyle("rgb(0, 0, 255)");
    canvas->beginPath();
    canvas->arc(cx, cy, r, M_PI * s, M_PI * e, 0);
    canvas->lineTo(cx, cy);
    canvas->closePath();
    canvas->fill();

    s = e;
    e = 270.0f/180.0f;
    canvas->setFillStyle("rgb(255, 255, 0)");
    canvas->beginPath();
    canvas->arc(cx, cy, r, M_PI * s, M_PI * e, 0);
    canvas->lineTo(cx, cy);
    canvas->closePath();
    canvas->fill();

    // Text enhancement
    canvas->setFillStyle("red");
    canvas->setFont("36pt Italic Liberation Serif");
    canvas->fillText("Hello, world.", 512, 200);

    canvas->setFillStyle("lime");
    canvas->setFont("40pt Bold Liberation Sans");
    canvas->fillText("Hello, world.", 512, 250);

    canvas->setFillStyle("blue");
    canvas->setFont("48pt Liberation Mono");
    canvas->fillText("Hello, world.", 512, 300);

    canvas->setFillStyle("fuchsia");
    canvas->setFont("48pt Sazanami Gothic");
    canvas->fillText("こんにちは、世界。", 512, 350);

    canvas->setFillStyle("aqua");
    canvas->setFont("48pt Sazanami Mincho");
    canvas->fillText("こんにちは、世界。", 512, 400);

}

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
    surface = cairo_image_surface_create_for_data(
        static_cast<u8*>(mapping), canvasInfo.format , canvasInfo.width, canvasInfo.height,
        sizeof(u32) * canvasInfo.width);
    Canvas* canvas = new Canvas(surface, canvasInfo.width, canvasInfo.height);
    ASSERT(canvas);
    Handle<es::Context> device = nameSpace->lookup("device");
    device->bind("canvas", static_cast<html5::CanvasRenderingContext2D*>(canvas));
    ASSERT(nameSpace->lookup("device/canvas"));

    esReport("start canvas.\n");

    if (argc < 2)
    {
        //figure(canvas);
	transform(canvas);
#ifndef __es__
        esSleep(50000000);
#endif
    }
    else
    {
        // Create a child process.
        Handle<es::Process> child = es::Process::createInstance();
        TEST(child);

        // Start the child process.
        std::string param;
        for (int i = 1; i < argc; ++i) {
            param += argv[i];
            param += " ";
        }
        Handle<es::File> file = nameSpace->lookup(argv[1]);
        if (file)
        {
            child->setRoot(nameSpace);
            child->setCurrent(nameSpace);
            child->start(file, param.c_str());
            child->wait();
        }
    }

    System()->unmap(mapping, framebuffer->getSize());

    canvas->release();

    esReport("quit canvas.\n");
}
