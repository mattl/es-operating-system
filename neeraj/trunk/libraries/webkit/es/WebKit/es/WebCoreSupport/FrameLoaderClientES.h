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

#ifndef FrameLoaderClientES_H
#define FrameLoaderClientES_H

#include "FrameLoaderClient.h"
#include "FrameLoader.h"
#include "KURL.h"
#include "ResourceResponse.h"

namespace WebCore {

class WebViewES;

struct LoadErrorResetToken;

class FrameLoaderClientES : public FrameLoaderClient {
public:
    FrameLoaderClientES();
    ~FrameLoaderClientES();

    virtual void frameLoaderDestroyed();

    virtual bool hasWebView() const;

    virtual void makeRepresentation(DocumentLoader*);
    virtual void forceLayout();
    virtual void forceLayoutForNonHTML();

    virtual void setCopiesOnScroll();

    virtual void detachedFromParent2();
    virtual void detachedFromParent3();

    virtual void assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&);

    virtual void dispatchWillSendRequest(DocumentLoader*, unsigned long identifier, ResourceRequest&, const ResourceResponse& redirectResponse);
    virtual bool shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier);
    virtual void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&);
    virtual void dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&);
    virtual void dispatchDidReceiveResponse(DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    virtual void dispatchDidReceiveContentLength(DocumentLoader*, unsigned long identifier, int lengthReceived);
    virtual void dispatchDidFinishLoading(DocumentLoader*, unsigned long identifier);
    virtual void dispatchDidFailLoading(DocumentLoader*, unsigned long identifier, const ResourceError&);
    virtual bool dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int length);
    virtual void dispatchDidLoadResourceByXMLHttpRequest(unsigned long identifier, const ScriptString&);

    virtual void dispatchDidHandleOnloadEvents();
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad();
    virtual void dispatchDidCancelClientRedirect();
    virtual void dispatchWillPerformClientRedirect(const KURL&, double interval, double fireDate);
    virtual void dispatchDidChangeLocationWithinPage();
    virtual void dispatchWillClose();
    virtual void dispatchDidReceiveIcon();
    virtual void dispatchDidStartProvisionalLoad();
    virtual void dispatchDidReceiveTitle(const String& title);
    virtual void dispatchDidCommitLoad();
    virtual void dispatchDidFailProvisionalLoad(const ResourceError&);
    virtual void dispatchDidFailLoad(const ResourceError&);
    virtual void dispatchDidFinishDocumentLoad();
    virtual void dispatchDidFinishLoad();
    virtual void dispatchDidFirstLayout();
    virtual void dispatchDidFirstVisuallyNonEmptyLayout();

    virtual Frame* dispatchCreatePage();
    virtual void dispatchShow();

    virtual void dispatchDecidePolicyForMIMEType(FramePolicyFunction, const String& MIMEType, const ResourceRequest&);
    virtual void dispatchDecidePolicyForNewWindowAction(FramePolicyFunction, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, const String& frameName);
    virtual void dispatchDecidePolicyForNavigationAction(FramePolicyFunction, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>);
    virtual void cancelPolicyCheck();

    virtual void dispatchUnableToImplementPolicy(const ResourceError&);

    virtual void dispatchWillSubmitForm(FramePolicyFunction, PassRefPtr<FormState>);

    virtual void dispatchDidLoadMainResource(DocumentLoader*);
    virtual void revertToProvisionalState(DocumentLoader*);
    virtual void setMainDocumentError(DocumentLoader*, const ResourceError&);

    virtual void willChangeEstimatedProgress() { }
    virtual void didChangeEstimatedProgress() { }
    virtual void postProgressStartedNotification();
    virtual void postProgressEstimateChangedNotification();
    virtual void postProgressFinishedNotification();

    virtual void setMainFrameDocumentReady(bool);

    virtual void startDownload(const ResourceRequest&);

    virtual void willChangeTitle(DocumentLoader*);
    virtual void didChangeTitle(DocumentLoader*);

    virtual void committedLoad(DocumentLoader*, const char*, int);
    virtual void finishedLoading(DocumentLoader*);

    virtual void updateGlobalHistory();
    virtual void updateGlobalHistoryRedirectLinks();

    virtual bool shouldGoToHistoryItem(HistoryItem*) const;

    virtual void didDisplayInsecureContent();

    virtual void didRunInsecureContent(SecurityOrigin*);

    virtual ResourceError cancelledError(const ResourceRequest&);
    virtual ResourceError blockedError(const ResourceRequest&);
    virtual ResourceError cannotShowURLError(const ResourceRequest&);
    virtual ResourceError interruptForPolicyChangeError(const ResourceRequest&);

    virtual ResourceError cannotShowMIMETypeError(const ResourceResponse&);
    virtual ResourceError fileDoesNotExistError(const ResourceResponse&);
    virtual ResourceError pluginWillHandleLoadError(const ResourceResponse&);

    virtual bool shouldFallBack(const ResourceError&);

    virtual bool canHandleRequest(const ResourceRequest&) const;
    virtual bool canShowMIMEType(const String& MIMEType) const;
    virtual bool representationExistsForURLScheme(const String& URLScheme) const;
    virtual String generatedMIMETypeForURLScheme(const String& URLScheme) const;

    virtual void frameLoadCompleted();
    virtual void saveViewStateToItem(HistoryItem*);
    virtual void restoreViewState();
    virtual void provisionalLoadStarted();
    virtual void didFinishLoad();
    virtual void prepareForDataSourceReplacement();

    virtual PassRefPtr<DocumentLoader> createDocumentLoader(const ResourceRequest&, const SubstituteData&);
    virtual void setTitle(const String& title, const KURL&);

    virtual String userAgent(const KURL&);

    virtual void savePlatformDataToCachedFrame(CachedFrame*);
    virtual void transitionToCommittedFromCachedFrame(CachedFrame*);
    virtual void transitionToCommittedForNewPage();

    virtual bool canCachePage() const;
    virtual void download(ResourceHandle*, const ResourceRequest&, const ResourceRequest&, const ResourceResponse&);

    virtual PassRefPtr<Frame> createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement,
                                const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight);
    virtual PassRefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&, bool loadManually);
    virtual void redirectDataToPlugin(Widget* pluginWidget);

    virtual PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const KURL& baseURL, const Vector<String>& paramNames, const Vector<String>& paramValues);

    virtual ObjectContentType objectContentType(const KURL& url, const String& mimeType);
    virtual String overrideMediaType() const;

    virtual void windowObjectCleared();
    virtual void documentElementAvailable();
    virtual void didPerformFirstNavigation() const;

    virtual void registerForIconNotification(bool listen = true);

    // ES
    void setFrame(Frame* frame);
    void setWebView(WebViewES* webview);

private:
    Frame *m_frame;
    WebViewES* m_webView;
    ResourceResponse m_response;
};

} // namespace WebCore

#endif
