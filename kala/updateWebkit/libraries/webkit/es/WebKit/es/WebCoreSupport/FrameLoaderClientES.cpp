/*
 * Copyright (C) 2007 Kevin Ollivier <kevino@theolliviers.com>
 * Copyright (c) 2009 Google Inc.
 * All rights reserved.
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
#include "FrameLoaderClientES.h"

#include "DocumentLoader.h"
#include "FormState.h"
#include "Frame.h"
#include "FrameLoaderTypes.h"
#include "FrameView.h"
#include "FrameTree.h"
#include "HTMLFormElement.h"
#include "HTMLFrameOwnerElement.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformString.h"
#include "ProgressTracker.h"
#include "RenderPart.h"
#include "ResourceError.h"
#include "ResourceResponse.h"
#include "ScriptController.h"
#include "ScriptString.h"


enum {
    WebKitErrorCannotShowMIMEType =                             100,
    WebKitErrorCannotShowURL =                                  101,
    WebKitErrorFrameLoadInterruptedByPolicyChange =             102,
};

enum {
    WebKitErrorCannotFindPlugIn =                               200,
    WebKitErrorCannotLoadPlugIn =                               201,
    WebKitErrorJavaUnavailable =                                202,
};

namespace WebCore {

FrameLoaderClientES::FrameLoaderClientES() :
    m_frame(0),
    m_webView(0)
{
}

FrameLoaderClientES::~FrameLoaderClientES()
{
}

void FrameLoaderClientES::setFrame(Frame *frame)
{
    m_frame = frame;
}

void FrameLoaderClientES::setWebView(WebViewES* webview)
{
    m_webView = webview;
}

void FrameLoaderClientES::frameLoaderDestroyed()
{
    m_frame = 0;
    delete this;
}

bool FrameLoaderClientES::hasWebView() const
{
    return m_webView != 0;
}

void FrameLoaderClientES::makeRepresentation(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClientES::forceLayout()
{
    notImplemented();
}

void FrameLoaderClientES::forceLayoutForNonHTML()
{
    notImplemented();
}

void FrameLoaderClientES::setCopiesOnScroll()
{
    notImplemented();
}

void FrameLoaderClientES::detachedFromParent2()
{
    notImplemented();
}

void FrameLoaderClientES::detachedFromParent3()
{
    notImplemented();
}

void FrameLoaderClientES::assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchWillSendRequest(DocumentLoader*, unsigned long, ResourceRequest& request, const ResourceResponse& response)
{
    notImplemented();
}

bool FrameLoaderClientES::shouldUseCredentialStorage(DocumentLoader*, unsigned long)
{
    notImplemented();
    return false;
}

void FrameLoaderClientES::dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge&)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge&)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidReceiveResponse(DocumentLoader* loader, unsigned long id, const ResourceResponse& response)
{
    notImplemented();
    m_response = response;
}

void FrameLoaderClientES::dispatchDidReceiveContentLength(DocumentLoader* loader, unsigned long id, int length)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidFinishLoading(DocumentLoader*, unsigned long)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidFailLoading(DocumentLoader*, unsigned long, const ResourceError&)
{
    notImplemented();
}

bool FrameLoaderClientES::dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int)
{
    notImplemented();
    return false;
}

void FrameLoaderClientES::dispatchDidLoadResourceByXMLHttpRequest(unsigned long, const ScriptString&)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidHandleOnloadEvents()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidCancelClientRedirect()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchWillPerformClientRedirect(const KURL&, double interval, double fireDate)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidChangeLocationWithinPage()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchWillClose()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidReceiveIcon()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidStartProvisionalLoad()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidReceiveTitle(const String& title)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidCommitLoad()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidFailProvisionalLoad(const ResourceError&)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidFailLoad(const ResourceError&)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidFinishDocumentLoad()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidFinishLoad()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidFirstLayout()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidFirstVisuallyNonEmptyLayout()
{
    notImplemented();
}

Frame* FrameLoaderClientES::dispatchCreatePage()
{
    notImplemented();
    return false;
}

void FrameLoaderClientES::dispatchShow()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDecidePolicyForMIMEType(FramePolicyFunction function, const String& mimetype, const ResourceRequest& request)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDecidePolicyForNewWindowAction(FramePolicyFunction function, const NavigationAction&, const ResourceRequest& request, PassRefPtr<FormState>, const String& targetName)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDecidePolicyForNavigationAction(FramePolicyFunction function, const NavigationAction& action, const ResourceRequest& request, PassRefPtr<FormState>)
{
    notImplemented();
}

void FrameLoaderClientES::cancelPolicyCheck()
{
    notImplemented();
}

void FrameLoaderClientES::dispatchUnableToImplementPolicy(const ResourceError&)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchWillSubmitForm(FramePolicyFunction function, PassRefPtr<FormState>)
{
    notImplemented();
}

void FrameLoaderClientES::dispatchDidLoadMainResource(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClientES::revertToProvisionalState(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClientES::setMainDocumentError(WebCore::DocumentLoader*, const WebCore::ResourceError&)
{
    notImplemented();
}

void FrameLoaderClientES::postProgressStartedNotification()
{
    notImplemented();
}

void FrameLoaderClientES::postProgressEstimateChangedNotification()
{
    notImplemented();
}

void FrameLoaderClientES::postProgressFinishedNotification()
{
    notImplemented();
}

void FrameLoaderClientES::setMainFrameDocumentReady(bool b)
{
    notImplemented();
}

void FrameLoaderClientES::startDownload(const ResourceRequest&)
{
    notImplemented();
}

void FrameLoaderClientES::willChangeTitle(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClientES::didChangeTitle(DocumentLoader *l)
{
    setTitle(l->title(), l->url());
}

void FrameLoaderClientES::committedLoad(WebCore::DocumentLoader* loader, const char* data, int length)
{
    notImplemented();
}

void FrameLoaderClientES::finishedLoading(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClientES::updateGlobalHistory()
{
    notImplemented();
}

void FrameLoaderClientES::updateGlobalHistoryRedirectLinks()
{
    notImplemented();
}

bool FrameLoaderClientES::shouldGoToHistoryItem(WebCore::HistoryItem*) const
{
    notImplemented();
    return true;
}

void FrameLoaderClientES::didDisplayInsecureContent()
{
    notImplemented();
}

void FrameLoaderClientES::didRunInsecureContent(SecurityOrigin* security) 
{
    notImplemented();
}	

WebCore::ResourceError FrameLoaderClientES::cancelledError(const WebCore::ResourceRequest& request)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorCannotShowURL, request.url().string(), "Load request cancelled");
}

WebCore::ResourceError FrameLoaderClientES::blockedError(const ResourceRequest& request)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorCannotShowURL, request.url().string(), "Not allowed to use restricted network port");
}

WebCore::ResourceError FrameLoaderClientES::cannotShowURLError(const WebCore::ResourceRequest& request)
{
    return ResourceError(String(), WebKitErrorCannotShowURL, request.url().string(), "URL cannot be shown");
}

WebCore::ResourceError FrameLoaderClientES::interruptForPolicyChangeError(const WebCore::ResourceRequest& request)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorFrameLoadInterruptedByPolicyChange, request.url().string(), "Frame load was interrupted");
}

WebCore::ResourceError FrameLoaderClientES::cannotShowMIMETypeError(const WebCore::ResourceResponse& response)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorCannotShowMIMEType, response.url().string(), "Content with the specified MIME type cannot be shown");
}

WebCore::ResourceError FrameLoaderClientES::fileDoesNotExistError(const WebCore::ResourceResponse& response)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorCannotShowURL, response.url().string(), "File does not exist");
}

ResourceError FrameLoaderClientES::pluginWillHandleLoadError(const ResourceResponse& response)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorCannotLoadPlugIn, response.url().string(), "Plugin will handle load");
}

bool FrameLoaderClientES::shouldFallBack(const WebCore::ResourceError& error)
{
    notImplemented();
    return false;
}

bool FrameLoaderClientES::canHandleRequest(const WebCore::ResourceRequest&) const
{
    notImplemented();
    return true;
}

bool FrameLoaderClientES::canShowMIMEType(const String& MIMEType) const
{
    notImplemented();
    return true;
}

bool FrameLoaderClientES::representationExistsForURLScheme(const String& URLScheme) const
{
    notImplemented();
    return false;
}

String FrameLoaderClientES::generatedMIMETypeForURLScheme(const String& URLScheme) const
{
    notImplemented();
    return String();
}

void FrameLoaderClientES::frameLoadCompleted()
{
    notImplemented();
}

void FrameLoaderClientES::saveViewStateToItem(HistoryItem*)
{
    notImplemented();
}

void FrameLoaderClientES::restoreViewState()
{
    notImplemented();
}

void FrameLoaderClientES::provisionalLoadStarted()
{
    notImplemented();
}

void FrameLoaderClientES::didFinishLoad()
{
    notImplemented();
}

void FrameLoaderClientES::prepareForDataSourceReplacement()
{
    notImplemented();
}

WTF::PassRefPtr<DocumentLoader> FrameLoaderClientES::createDocumentLoader(const ResourceRequest& request, const SubstituteData& substituteData)
{
    return DocumentLoader::create(request, substituteData);
}

void FrameLoaderClientES::setTitle(const String& title, const KURL&)
{
    notImplemented();
}

String FrameLoaderClientES::userAgent(const KURL&)
{
    return String("Mozilla/5.0 (PC; U; ES; en) AppleWebKit/528.5+ (KHTML, like Gecko) Safari/528.5+");
}

void FrameLoaderClientES::savePlatformDataToCachedFrame(CachedFrame*)
{
    notImplemented();
}

void FrameLoaderClientES::transitionToCommittedFromCachedFrame(CachedFrame*)
{
    notImplemented();
}

void FrameLoaderClientES::transitionToCommittedForNewPage()
{
    ASSERT(m_frame);
    ASSERT(m_webView);

    Page* page = m_frame->page();
    ASSERT(page);

    bool isMainFrame = m_frame == page->mainFrame();

    m_frame->setView(0);

    PassRefPtr<FrameView> frameView;
    if (isMainFrame)
        frameView = FrameView::create(m_frame , IntSize(1024, 768));
    else
        frameView = FrameView::create(m_frame);

    frameView->setCanHaveScrollbars(false);  // Turn off scrollbars

    m_frame->setView(frameView.get());

    if (HTMLFrameOwnerElement* owner = m_frame->ownerElement())
        m_frame->view()->setScrollbarModes(owner->scrollingMode(), owner->scrollingMode());
}

bool FrameLoaderClientES::canCachePage() const
{
    notImplemented();
    return false;
}

void FrameLoaderClientES::download(ResourceHandle*, const ResourceRequest&, const ResourceRequest&, const ResourceResponse&)
{
    notImplemented();
}

PassRefPtr<Frame> FrameLoaderClientES::createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement, const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight)
{
    notImplemented();
    return 0;
}

PassRefPtr<Widget> FrameLoaderClientES::createPlugin(const IntSize&, HTMLPlugInElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&, bool loadManually)
{
    notImplemented();
    return 0;
}

void FrameLoaderClientES::redirectDataToPlugin(Widget* pluginWidget)
{
    notImplemented();
    return;
}

PassRefPtr<Widget> FrameLoaderClientES::createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const KURL& baseURL,
                                                    const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    notImplemented();
    return 0;
}

ObjectContentType FrameLoaderClientES::objectContentType(const KURL& url, const String& mimeType)
{
    notImplemented();
    return ObjectContentType();
}

String FrameLoaderClientES::overrideMediaType() const
{
    notImplemented();
    return String();
}

void FrameLoaderClientES::windowObjectCleared()
{
    notImplemented();
}

void FrameLoaderClientES::documentElementAvailable()
{
    notImplemented();
}

void FrameLoaderClientES::didPerformFirstNavigation() const
{
    notImplemented();
}

void FrameLoaderClientES::registerForIconNotification(bool listen)
{
    notImplemented();
}

} // namespace WebCore
