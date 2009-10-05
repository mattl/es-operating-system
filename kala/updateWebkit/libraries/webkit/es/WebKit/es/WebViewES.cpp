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
#include "Element.h"
#include "Editor.h"
#include "EventHandler.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "Logging.h"
#include "markup.h"
#include "Page.h"
#include "ContextMenu.h"
#include "ContextMenuItem.h"
#include "ContextMenuController.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PlatformString.h"
#include "PlatformWheelEvent.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "Scrollbar.h"
#include "SelectionController.h"
#include "Settings.h"
#include "SubstituteData.h"

#include "ChromeClientES.h"
#include "ContextMenuClientES.h"
#include "DragClientES.h"
#include "EditorClientES.h"
#include "FrameLoaderClientES.h"
#include "InspectorClientES.h"

#include "WebFrameES.h"
#include "WebViewES.h"

namespace WebCore {

WebViewES::WebViewES()
{
    HTMLFrameOwnerElement* parentFrame = 0;

    EditorClientES* editorClient = new EditorClientES();
    page = new Page(new ChromeClientES(), new ContextMenuClientES(), editorClient, new DragClientES(), new InspectorClientES(), NULL);

    mainFrame = new WebFrameES(this);

    // Default settings - we should have WebViewESSettings class for this
    // eventually
    WebCore::Settings* settings = page->settings();
    settings->setLoadsImagesAutomatically(true);
    settings->setDefaultFixedFontSize(13);
    settings->setDefaultFontSize(16);
    settings->setSerifFontFamily("Times New Roman");
    settings->setFixedFontFamily("Courier New");
    settings->setSansSerifFontFamily("Arial");
    settings->setStandardFontFamily("Times New Roman");
    settings->setJavaScriptEnabled(true);
}

WebViewES::~WebViewES()
{
    delete mainFrame;
    delete page;
    page = 0;
}

void WebViewES::stop()
{
    if (mainFrame)
        mainFrame->stop();
}

void WebViewES::reload()
{
    if (mainFrame)
        mainFrame->reload();
}

bool WebViewES::goBack()
{
    if (mainFrame)
        return mainFrame->goBack();

    return false;
}

bool WebViewES::goForward()
{
    if (mainFrame)
        return mainFrame->goForward();

    return false;
}

bool WebViewES::canGoBack()
{
    if (mainFrame)
        return mainFrame->canGoBack();

    return false;
}

bool WebViewES::canGoForward()
{
    if (mainFrame)
        return mainFrame->canGoForward();

    return false;
}

bool WebViewES::canCopy()
{
    if (mainFrame)
        return mainFrame->canCopy();

    return false;
}

void WebViewES::copy()
{
    if (mainFrame)
        mainFrame->copy();
}

bool WebViewES::canCut()
{
    if (mainFrame)
        return mainFrame->canCut();

    return false;
}

void WebViewES::cut()
{
    if (mainFrame)
        mainFrame->cut();
}

bool WebViewES::canPaste()
{
    if (mainFrame)
        return mainFrame->canPaste();

    return false;
}

void WebViewES::paste()
{
    if (mainFrame)
        mainFrame->paste();
}

} // namespace WebCore
