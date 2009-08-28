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
#include "SVGLineElement.h"
#include "SVGTextElement.h"
#include "SVGTextContentElement.h"
#include "Text.h"
#include "SVGPathElement.h"
#include "SVGPathSeg.h"
#include "SVGPathSegMoveto.h"
#include "SVGPathSegArc.h"
#include "SVGPathSegClosePath.h"
#include "SVGPathSegCurvetoCubic.h"
#include "SVGPathSegLineto.h"

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

    WTF::PassRefPtr<WebCore::SVGRectElement> rectElement_red = WTF::static_pointer_cast<WebCore::SVGRectElement>(document->createElementNS("http://www.w3.org/2000/svg", "rect", ec));
    rectElement_red->setAttribute("x", "90", ec);
    rectElement_red->setAttribute("y", "75", ec);
    rectElement_red->setAttribute("width", "20", ec);
    rectElement_red->setAttribute("height", "85", ec);
    rectElement_red->setAttribute("fill", "#ff0000", ec);
    rootElement->appendChild(rectElement_red.get(), ec);

    WTF::PassRefPtr<WebCore::SVGRectElement> rectElement_green = WTF::static_pointer_cast<WebCore::SVGRectElement>(document->createElementNS("http://www.w3.org/2000/svg", "rect", ec));
    rectElement_green->setAttribute("x", "125", ec);
    rectElement_green->setAttribute("y", "35", ec);
    rectElement_green->setAttribute("width", "20", ec);
    rectElement_green->setAttribute("height", "125", ec);
    rectElement_green->setAttribute("fill", "#00ff00", ec);
    rootElement->appendChild(rectElement_green.get(), ec);

    WTF::PassRefPtr<WebCore::SVGRectElement> rectElement_blue = WTF::static_pointer_cast<WebCore::SVGRectElement>(document->createElementNS("http://www.w3.org/2000/svg", "rect", ec));
    rectElement_blue->setAttribute("x", "160", ec);
    rectElement_blue->setAttribute("y", "90", ec);
    rectElement_blue->setAttribute("width", "20", ec);
    rectElement_blue->setAttribute("height", "70", ec);
    rectElement_blue->setAttribute("fill", "#0000ff", ec);
    rootElement->appendChild(rectElement_blue.get(), ec);

    WTF::PassRefPtr<WebCore::SVGRectElement> rectElement_yellow = WTF::static_pointer_cast<WebCore::SVGRectElement>(document->createElementNS("http://www.w3.org/2000/svg", "rect", ec));
    rectElement_yellow->setAttribute("x", "195", ec);
    rectElement_yellow->setAttribute("y", "110", ec);
    rectElement_yellow->setAttribute("width", "20", ec);
    rectElement_yellow->setAttribute("height", "50", ec);
    rectElement_yellow->setAttribute("fill", "#ffff00", ec);
    rootElement->appendChild(rectElement_yellow.get(), ec);

    WTF::PassRefPtr<WebCore::SVGRectElement> rectElement_blue2 = WTF::static_pointer_cast<WebCore::SVGRectElement>(document->createElementNS("http://www.w3.org/2000/svg", "rect", ec));
    rectElement_blue2->setAttribute("x", "235", ec);
    rectElement_blue2->setAttribute("y", "120", ec);
    rectElement_blue2->setAttribute("width", "20", ec);
    rectElement_blue2->setAttribute("height", "40", ec);
    rectElement_blue2->setAttribute("fill", "#00ffff", ec);
    rootElement->appendChild(rectElement_blue2.get(), ec);

    WTF::PassRefPtr<WebCore::SVGLineElement> lineElement_1 = WTF::static_pointer_cast<WebCore::SVGLineElement>(document->createElementNS("http://www.w3.org/2000/svg", "line", ec));
    lineElement_1->setAttribute("x1", "75", ec);
    lineElement_1->setAttribute("y1", "160", ec);
    lineElement_1->setAttribute("x2", "281", ec);
    lineElement_1->setAttribute("y2", "160", ec);
    lineElement_1->setAttribute("stroke-width", "3", ec);
    lineElement_1->setAttribute("stroke", "black", ec);
    rootElement->appendChild(lineElement_1.get(), ec);

    WTF::PassRefPtr<WebCore::SVGLineElement> lineElement_2 = WTF::static_pointer_cast<WebCore::SVGLineElement>(document->createElementNS("http://www.w3.org/2000/svg", "line", ec));
    lineElement_2->setAttribute("x1", "75", ec);
    lineElement_2->setAttribute("y1", "160", ec);
    lineElement_2->setAttribute("x2", "77", ec);
    lineElement_2->setAttribute("y2", "30", ec);
    lineElement_2->setAttribute("stroke-width", "3", ec);
    lineElement_2->setAttribute("stroke", "black", ec);
    rootElement->appendChild(lineElement_2.get(), ec);

    WTF::PassRefPtr<WebCore::SVGTextElement> textElement_1 = WTF::static_pointer_cast<WebCore::SVGTextElement>(document->createElementNS("http://www.w3.org/2000/svg", "text", ec));
    textElement_1->setAttribute("x", "512", ec);
    textElement_1->setAttribute("y", "200", ec);
    textElement_1->setAttribute("font-size", "36", ec);
    textElement_1->setAttribute("font-style", "italic", ec);
    textElement_1->setAttribute("font-family", "Liberation Serif", ec);
    textElement_1->setAttribute("fill", "#ff0000", ec);
    WTF::PassRefPtr<WebCore::Text> textNode_1 = WTF::static_pointer_cast<WebCore::Text>(document->createTextNode("Hello World"));
    textElement_1->appendChild(textNode_1.get(), ec);
    rootElement->appendChild(textElement_1.get(), ec);

    WTF::PassRefPtr<WebCore::SVGTextElement> textElement_2 = WTF::static_pointer_cast<WebCore::SVGTextElement>(document->createElementNS("http://www.w3.org/2000/svg", "text", ec));
    textElement_2->setAttribute("x", "512", ec);
    textElement_2->setAttribute("y", "250", ec);
    textElement_2->setAttribute("font-size", "40", ec);
    textElement_2->setAttribute("font-weight", "bold", ec);
    textElement_2->setAttribute("font-family", "Liberation Sans", ec);
    textElement_2->setAttribute("fill", "#00ff00", ec);
    WTF::PassRefPtr<WebCore::Text> textNode_2 = WTF::static_pointer_cast<WebCore::Text>(document->createTextNode("Hello World"));
    textElement_2->appendChild(textNode_2.get(), ec);
    rootElement->appendChild(textElement_2.get(), ec);

    WTF::PassRefPtr<WebCore::SVGTextElement> textElement_3 = WTF::static_pointer_cast<WebCore::SVGTextElement>(document->createElementNS("http://www.w3.org/2000/svg", "text", ec));
    textElement_3->setAttribute("x", "512", ec);
    textElement_3->setAttribute("y", "300", ec);
    textElement_3->setAttribute("font-size", "48", ec);
    textElement_3->setAttribute("font-family", "Liberation Mono", ec);
    textElement_3->setAttribute("fill", "#0000ff", ec);
    WTF::PassRefPtr<WebCore::Text> textNode_3 = WTF::static_pointer_cast<WebCore::Text>(document->createTextNode("Hello World"));
    textElement_3->appendChild(textNode_3.get(), ec);
    rootElement->appendChild(textElement_3.get(), ec);

    WTF::PassRefPtr<WebCore::SVGPathElement> pathElement = WTF::static_pointer_cast<WebCore::SVGPathElement>(document->createElementNS("http://www.w3.org/2000/svg", "path", ec));
    pathElement->createSVGPathSegMovetoAbs(494,296);
    pathElement->createSVGPathSegCurvetoCubicAbs(500,295,506,294,512,294);
    pathElement->createSVGPathSegCurvetoCubicAbs(511,328,512,361,511,394);
    pathElement->createSVGPathSegCurvetoCubicAbs(511,393,510,392,510,392);
    pathElement->createSVGPathSegLinetoAbs(511,393);
    pathElement->createSVGPathSegCurvetoCubicAbs(506,360,499,328,494,296);
    pathElement->createSVGPathSegClosePath();
    pathElement->setAttribute("fill", "#edfa16", ec);
    rootElement->appendChild(pathElement.get(), ec);

    if (frame->contentRenderer() && frame->view())
    {
        WebCore::GraphicsContext ctx(cr);
        WebCore::IntRect rect(0, 0, 1024, 768);
        frame->view()->layoutIfNeededRecursive();
        frame->view()->paint(&ctx, rect);
    }
}
