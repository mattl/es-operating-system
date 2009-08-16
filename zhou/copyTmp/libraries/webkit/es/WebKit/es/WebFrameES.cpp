/*
 * Copyright (C) 2007 Kevin Ollivier  All rights reserved.
 * Copyright (c) 2009, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CString.h"
#include "Document.h"
#include "Editor.h"
#include "Element.h"
#include "EventHandler.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "HitTestResult.h"
#include "HTMLFrameOwnerElement.h"
#include "markup.h"
#include "Page.h"
#include "RenderTreeAsText.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "ScriptController.h"
#include "ScriptValue.h"
#include "TextEncoding.h"

#include "EditorClientES.h"
#include "FrameLoaderClientES.h"

#include "WebFrameES.h"
#include "WebViewES.h"

namespace WebCore {

WebFrameES::WebFrameES(WebViewES* container, WebFrameES* parent, HTMLFrameOwnerElement* ownerElement)
{
    FrameLoaderClientES* loaderClient = new FrameLoaderClientES();
    frame = WebCore::Frame::create(container->getPage(), ownerElement, loaderClient);
    frame->deref();

    loaderClient->setFrame(frame.get());
    loaderClient->setWebView(container);

    frame->init();
}

WebFrameES::~WebFrameES()
{
    frame->loader()->detachFromParent();
}

WebCore::Frame* WebFrameES::getFrame()
{
    return frame.get();
}

void WebFrameES::stop()
{
    if (frame && frame->loader())
        frame->loader()->stop();
}

void WebFrameES::reload()
{
    if (frame && frame->loader())
        frame->loader()->reload();
}

bool WebFrameES::goBack()
{
    if (frame && frame->page())
        return frame->page()->goBack();

    return false;
}

bool WebFrameES::goForward()
{
    if (frame && frame->page())
        return frame->page()->goForward();

    return false;
}

bool WebFrameES::canGoBack()
{
    if (frame && frame->page() && frame->page()->backForwardList())
        return frame->page()->backForwardList()->backItem() != NULL;

    return false;
}

bool WebFrameES::canGoForward()
{
    if (frame && frame->page() && frame->page()->backForwardList())
        return frame->page()->backForwardList()->forwardItem() != NULL;

    return false;
}

void WebFrameES::undo()
{
    if (frame && frame->editor() && canUndo())
        return frame->editor()->undo();
}

void WebFrameES::redo()
{
    if (frame && frame->editor() && canRedo())
        return frame->editor()->redo();
}

bool WebFrameES::canUndo()
{
    if (frame && frame->editor())
        return frame->editor()->canUndo();

    return false;
}

bool WebFrameES::canRedo()
{
    if (frame && frame->editor())
        return frame->editor()->canRedo();

    return false;
}

bool WebFrameES::canCopy()
{
    if (frame && frame->view())
        return (frame->editor()->canCopy() || frame->editor()->canDHTMLCopy());

    return false;
}

void WebFrameES::copy()
{
    if (canCopy())
        frame->editor()->copy();
}

bool WebFrameES::canCut()
{
    if (frame && frame->view())
        return (frame->editor()->canCut() || frame->editor()->canDHTMLCut());

    return false;
}

void WebFrameES::cut()
{
    if (canCut())
        frame->editor()->cut();
}

bool WebFrameES::canPaste()
{
    if (frame && frame->view())
        return (frame->editor()->canPaste() || frame->editor()->canDHTMLPaste());

    return false;
}

void WebFrameES::paste()
{
    if (canPaste())
        frame->editor()->paste();
}

} // namespace WebCore
