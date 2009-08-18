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

#include "SVGDocument.h"
#include "SVGNames.h"
#include "SVGSVGElement.h"
#include "SVGRectElement.h"

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

void testCanvas2d(cairo_t* cr)
{
    WebCore::ExceptionCode ec = 0;

    WTF::initializeThreading();

    WebCore::WebViewES* webView = new WebCore::WebViewES;

    WebCore::Frame* frame = webView->getMainFrame()->getFrame();

    // A document have been created already in frame.
    WebCore::SVGDocument* document = dynamic_cast<WebCore::SVGDocument*>(frame->document());
    printf("SVGDocument* %p\n", document);

    // Create and append the svg element
    WebCore::SVGSVGElement* rootElement = document->rootElement();
    if (!rootElement) {
        ec = 0;
        rootElement = new WebCore::SVGSVGElement(WebCore::SVGNames::svgTag, document);
        document->appendChild(rootElement, ec);
    }
    rootElement = document->rootElement();
    printf("SVGSVGElement* %p\n", rootElement);

    // Create and append a rect element
    ec = 0;
    WTF::PassRefPtr<WebCore::SVGRectElement> rectElement = WTF::static_pointer_cast<WebCore::SVGRectElement>(document->createElementNS("http://www.w3.org/2000/svg", "rect", ec));
    rectElement->setAttributeNS(WebCore::nullAtom, "x", "10", ec);
    rectElement->setAttributeNS(WebCore::nullAtom, "y", "10", ec);
    rectElement->setAttributeNS(WebCore::nullAtom, "width", "200", ec);
    rectElement->setAttributeNS(WebCore::nullAtom, "height", "200", ec);

    rootElement->appendChild(rectElement.get(), ec);

    if (frame->contentRenderer() && frame->view())
    {
        WebCore::GraphicsContext ctx(cr);
        WebCore::IntRect rect(0, 0, 1024, 768);
        frame->view()->layoutIfNeededRecursive();
        frame->view()->paint(&ctx, rect);
    }
}
