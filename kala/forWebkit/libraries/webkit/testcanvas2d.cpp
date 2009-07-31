/*
 * Copyright (c) 2009, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CanvasRenderingContext2D.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLBodyElement.h"
#include "HTMLCanvasElement.h"
#include "HTMLDocument.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "Page.h"
#include "QualifiedName.h"
#include "CString.h"
#include "Threading.h"

#include "ChromeClientES.h"
#include "ContextMenuClientES.h"
#include "EditorClientES.h"
#include "DragClientES.h"
#include "FrameLoaderClientES.h"
#include "InspectorClientES.h"

#include "WebFrameES.h"
#include "WebViewES.h"

#include "html5_canvasrenderingcontext2d.h"

#include <es.h>

void figure(es::CanvasRenderingContext2D* canvas)
{
    // Bar graph
    float top = 50.0f;
    float bottom = 250.0f;

    float width = 20.0f;
    float height;
    float x;
    float y;

    esReport("Start figure.\n");

    canvas->setStrokeStyle("rgb(0, 0, 0)");
    canvas->setLineWidth(3);

    height = 100.0f;
    x = 100.0f;
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

    esReport("End figure.\n");
}

void testCanvas2d(cairo_t* cr)
{
    WebCore::ExceptionCode ec = 0;

    esReport("start init.\n");

    WTF::initializeThreading();

    esReport("finished init.\n");

    WebCore::WebViewES* webView = new WebCore::WebViewES;

    WebCore::Frame* frame = webView->getMainFrame()->getFrame();

    esReport("Dcl element.\n");

    // A document have been created already in frame with an HTML element.
    WebCore::HTMLDocument* document = dynamic_cast<WebCore::HTMLDocument*>(frame->document());

    esReport("Create body.\n");

    // Create and append the body
    if (WebCore::Node* documentElement = document->documentElement()) {
        ec = 0;
        documentElement->appendChild(new WebCore::HTMLBodyElement(WebCore::HTMLNames::bodyTag, document), ec);
    }

    esReport("Create canvas element.\n");

    // Create and append the canvas element
    ec = 0;
    WTF::PassRefPtr<WebCore::HTMLCanvasElement> canvasElement = WTF::static_pointer_cast<WebCore::HTMLCanvasElement>(document->createElement("canvas", ec));
    canvasElement->setWidth(1024);
    canvasElement->setHeight(768);

    esReport("Append Child.\n");

    WebCore::HTMLElement* body = document->body();
    body->appendChild(canvasElement.get(), ec);

    WebCore::CanvasRenderingContext2D* canvasRenderingContext = canvasElement->getContext(WebCore::String::fromUTF8("2d"));

    CanvasRenderingContext2D_Impl canvasImpl(canvasRenderingContext);
    
    esReport("Start Figure.\n");

    figure(&canvasImpl);

    if (frame->contentRenderer() && frame->view())
    {
	esReport("Paint view.\n");

        WebCore::GraphicsContext ctx(cr);
        WebCore::IntRect rect(0, 0, 1024, 768);
        frame->view()->layoutIfNeededRecursive();
        frame->view()->paint(&ctx, rect);
    }

    esReport("Finish testcanvas2d.\n");
}
