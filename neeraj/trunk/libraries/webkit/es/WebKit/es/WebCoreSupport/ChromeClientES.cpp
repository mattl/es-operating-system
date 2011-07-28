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

#include "config.h"
#include "ChromeClientES.h"

#include "Console.h"
#include "FileChooser.h"
#include "FloatRect.h"
#include "FrameLoadRequest.h"
#include "NotImplemented.h"
#include "PlatformString.h"

namespace WebCore {

ChromeClientES::ChromeClientES()
{
}

ChromeClientES::~ChromeClientES()
{
}

void ChromeClientES::chromeDestroyed()
{
    notImplemented();
}

void ChromeClientES::setWindowRect(const FloatRect&)
{
    notImplemented();
}

FloatRect ChromeClientES::windowRect()
{
    notImplemented();
    return FloatRect();
}

FloatRect ChromeClientES::pageRect()
{
    notImplemented();
    return FloatRect();
}

float ChromeClientES::scaleFactor()
{
    notImplemented();
    return 1.0f;
}

void ChromeClientES::focus()
{
    notImplemented();
}

void ChromeClientES::unfocus()
{
    notImplemented();
}

bool ChromeClientES::canTakeFocus(FocusDirection)
{
    notImplemented();
    return false;
}

void ChromeClientES::takeFocus(FocusDirection)
{
    notImplemented();
}


Page* ChromeClientES::createWindow(Frame*, const FrameLoadRequest& request, const WindowFeatures&)
{
    notImplemented();
    return 0;
}

void ChromeClientES::show()
{
    notImplemented();
}

bool ChromeClientES::canRunModal()
{
    notImplemented();
    return false;
}

void ChromeClientES::runModal()
{
    notImplemented();
}

void ChromeClientES::setToolbarsVisible(bool)
{
    notImplemented();
}

bool ChromeClientES::toolbarsVisible()
{
    notImplemented();
    return false;
}

void ChromeClientES::setStatusbarVisible(bool)
{
    notImplemented();
}

bool ChromeClientES::statusbarVisible()
{
    notImplemented();
    return false;
}

void ChromeClientES::setScrollbarsVisible(bool)
{
    notImplemented();
}

bool ChromeClientES::scrollbarsVisible()
{
    notImplemented();
    return false;
}

void ChromeClientES::setMenubarVisible(bool)
{
    notImplemented();
}

bool ChromeClientES::menubarVisible()
{
    notImplemented();
    return false;
}

void ChromeClientES::setResizable(bool)
{
    notImplemented();
}

void ChromeClientES::addMessageToConsole(MessageSource source,
					 MessageType type,
                                         MessageLevel level,
                                         const String& message,
                                         unsigned int lineNumber,
                                         const String& sourceID)
{
    notImplemented();
}

bool ChromeClientES::canRunBeforeUnloadConfirmPanel()
{
    notImplemented();
    return true;
}

bool ChromeClientES::runBeforeUnloadConfirmPanel(const String& string,
                                                 Frame* frame)
{
    notImplemented();
    return true;
}

void ChromeClientES::closeWindowSoon()
{
    notImplemented();
}

void ChromeClientES::runJavaScriptAlert(Frame* frame, const String& string)
{
    notImplemented();
}

bool ChromeClientES::runJavaScriptConfirm(Frame* frame, const String& string)
{
    notImplemented();
    return true;
}

bool ChromeClientES::runJavaScriptPrompt(Frame* frame, const String& message, const String& defaultValue, String& result)
{
    notImplemented();
    return true;
}

void ChromeClientES::setStatusbarText(const String&)
{
    notImplemented();
}

bool ChromeClientES::shouldInterruptJavaScript()
{
    notImplemented();
    return false;
}

bool ChromeClientES::tabsToLinks() const
{
    notImplemented();
    return false;
}

IntRect ChromeClientES::windowResizerRect() const
{
    notImplemented();
    return IntRect();
}

void ChromeClientES::repaint(const IntRect& rect, bool contentChanged, bool immediate, bool repaintContentOnly)
{
    notImplemented();
}

void ChromeClientES::scroll(const IntSize&, const IntRect&, const IntRect&)
{
    notImplemented();
}

IntRect ChromeClientES::windowToScreen(const IntRect& rect) const
{
    notImplemented();
    return rect;
}

IntPoint ChromeClientES::screenToWindow(const IntPoint& point) const
{
    notImplemented();
    return point;
}

PlatformWidget ChromeClientES::platformPageClient() const
{
    notImplemented();
    return 0;
}

void ChromeClientES::contentsSizeChanged(Frame*, const IntSize&) const
{
    notImplemented();
}

void ChromeClientES::scrollRectIntoView(const IntRect&, const ScrollView*) const
{
    notImplemented();
}

void ChromeClientES::scrollbarsModeDidChange() const
{
    notImplemented();
}

void ChromeClientES::mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags)
{
    notImplemented();
}

void ChromeClientES::setToolTip(const String& tip)
{
    notImplemented();
}

void ChromeClientES::setToolTip(const String&, TextDirection)
{
    notImplemented();
}

void ChromeClientES::print(Frame*)
{
    notImplemented();
}

#if ENABLE(DATABASE)
void ChromeClientES::exceededDatabaseQuota(Frame*, const String&)
{
    notImplemented();
}
#endif

void ChromeClientES::requestGeolocationPermissionForFrame(Frame*, Geolocation*)
{
    notImplemented();
}

void ChromeClientES::runOpenPanel(Frame*, PassRefPtr<FileChooser>)
{
    notImplemented();
}

bool ChromeClientES::setCursor(PlatformCursorHandle)
{
    notImplemented();
    return false;
}

void ChromeClientES::formStateDidChange(const Node*)
{
    notImplemented();
}

PassOwnPtr<HTMLParserQuirks> ChromeClientES::createHTMLParserQuirks()
{
    notImplemented();
    return 0;
}

} // namespace WebCore
