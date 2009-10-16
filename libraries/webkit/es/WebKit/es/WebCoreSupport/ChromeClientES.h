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

#ifndef ChromeClientES_H
#define ChromeClientES_H

#include "ChromeClient.h"

namespace WebCore {

class ChromeClientES : public ChromeClient {
public:
    ChromeClientES();
    virtual ~ChromeClientES();

    virtual void chromeDestroyed();

    virtual void setWindowRect(const FloatRect&);
    virtual FloatRect windowRect();

    virtual FloatRect pageRect();

    virtual float scaleFactor();

    virtual void focus();
    virtual void unfocus();

    virtual bool canTakeFocus(FocusDirection);
    virtual void takeFocus(FocusDirection);

    virtual Page* createWindow(Frame*, const FrameLoadRequest&, const WindowFeatures&);
    virtual void show();

    virtual bool canRunModal();
    virtual void runModal();

    virtual void setToolbarsVisible(bool);
    virtual bool toolbarsVisible();

    virtual void setStatusbarVisible(bool);
    virtual bool statusbarVisible();

    virtual void setScrollbarsVisible(bool);
    virtual bool scrollbarsVisible();

    virtual void setMenubarVisible(bool);
    virtual bool menubarVisible();

    virtual void setResizable(bool);

    virtual void addMessageToConsole(MessageSource, MessageType, MessageLevel, const String& message, unsigned int lineNumber, const String& sourceID);

    virtual bool canRunBeforeUnloadConfirmPanel();
    virtual bool runBeforeUnloadConfirmPanel(const String& message, Frame* frame);

    virtual void closeWindowSoon();

    virtual void runJavaScriptAlert(Frame*, const String&);
    virtual bool runJavaScriptConfirm(Frame*, const String&);
    virtual bool runJavaScriptPrompt(Frame*, const String& message, const String& defaultValue, String& result);
    virtual void setStatusbarText(const String&);
    virtual bool shouldInterruptJavaScript();
    virtual bool tabsToLinks() const;

    virtual IntRect windowResizerRect() const;

    virtual void repaint(const IntRect&, bool contentChanged, bool immediate = false, bool repaintContentOnly = false);
    virtual void scroll(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect);
    virtual IntPoint screenToWindow(const IntPoint&) const;
    virtual IntRect windowToScreen(const IntRect&) const;
    virtual PlatformPageClient platformPageClient() const;
    virtual void contentsSizeChanged(Frame*, const IntSize&) const;
    virtual void scrollRectIntoView(const IntRect&, const ScrollView*) const;

    virtual void scrollbarsModeDidChange() const;
    virtual void mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags);

    virtual void setToolTip(const String&);
    virtual void setToolTip(const String&, TextDirection);

    virtual void print(Frame*);

#if ENABLE(DATABASE)
    virtual void exceededDatabaseQuota(Frame*, const String& databaseName);
#endif

    virtual void requestGeolocationPermissionForFrame(Frame*, Geolocation*);

    virtual void runOpenPanel(Frame*, PassRefPtr<FileChooser>);

    virtual bool setCursor(PlatformCursorHandle);

    virtual void formStateDidChange(const Node*);

    virtual PassOwnPtr<HTMLParserQuirks> createHTMLParserQuirks();

#if USE(ACCELERATED_COMPOSITING)
    virtual void attachRootGraphicsLayer(Frame*, GraphicsLayer*);
    virtual void setNeedsOneShotDrawingSynchronization();
    virtual void scheduleViewUpdate();
#endif
};

} // namespace WebCore

#endif // ChromeClientES_H
