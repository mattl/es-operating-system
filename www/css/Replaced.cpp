/*
 * Copyright 2010, 2011 Esrille Inc.
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

#include "Box.h"

#include <org/w3c/dom/html/HTMLIFrameElement.h>
#include <org/w3c/dom/html/HTMLImageElement.h>

#include "ViewCSSImp.h"
#include "WindowImp.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

void Box::resolveReplacedWidth(float intrinsicWidth, float intrinsicHeight)
{
    if (style->width.isAuto() && style->height.isAuto()) {
        if (0.0f <= intrinsicWidth)
            width = intrinsicWidth;
        else
            width = 300.0f;
        if (0.0f <= intrinsicHeight)
            height = intrinsicHeight;
        else
            height = 150.0f;
    } else if (style->width.isAuto()) {
        height = style->height.getPx();
        if (0.0f < intrinsicHeight && 0.0f <= intrinsicWidth)
            width = height * intrinsicWidth / intrinsicHeight;
        else
            width = 300.0f;
    } else if (style->height.isAuto()) {
        width = style->width.getPx();
        if (0.0f <= intrinsicHeight && 0.0f < intrinsicWidth)
            height = width * intrinsicHeight / intrinsicWidth;
        else
            height = 150.0f;
    } else {
        width = style->width.getPx();
        height = style->height.getPx();
    }
    intrinsic = true;
}

void BlockLevelBox::
layOutReplacedElement(ViewCSSImp* view, Box* replaced, Element element, CSSStyleDeclarationImp* style)
{
    float intrinsicWidth = 0.0f;
    float intrinsicHeight = 0.0f;

    std::u16string tag = element.getLocalName();
    if (tag == u"img") {
        html::HTMLImageElement img = interface_cast<html::HTMLImageElement>(element);
        if (BoxImage* backgroundImage = new(std::nothrow) BoxImage(replaced, view->getDocument().getDocumentURI(), img)) {
            replaced->backgroundImage = backgroundImage;
            intrinsicWidth = backgroundImage->getWidth();
            intrinsicHeight = backgroundImage->getHeight();
        }
    } else if (tag == u"iframe") {
        html::HTMLIFrameElement iframe = interface_cast<html::HTMLIFrameElement>(element);
        replaced->width = CSSTokenizer::parseInt(iframe.getWidth().c_str(), iframe.getWidth().size());
        replaced->height = CSSTokenizer::parseInt(iframe.getHeight().c_str(), iframe.getHeight().size());
        html::Window contentWindow = iframe.getContentWindow();
        if (WindowImp* imp = dynamic_cast<WindowImp*>(contentWindow.self())) {
            imp->setSize(replaced->width, replaced->height);
            replaced->shadow = imp->getView();
            std::u16string src = iframe.getSrc();
            if (!src.empty() && !imp->getDocument())
                iframe.setSrc(src);
        }
    }

    replaced->resolveReplacedWidth(intrinsicWidth, intrinsicHeight);
}

}}}}  // org::w3c::dom::bootstrap
